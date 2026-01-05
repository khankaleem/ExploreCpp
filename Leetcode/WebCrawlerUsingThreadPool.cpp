/**
 * // This is the HtmlParser's API interface.
 * // You should not implement it, or speculate about its implementation
 * class HtmlParser {
 *   public:
 *     vector<string> getUrls(string url);
 * };
 */

class ThreadPool {
public:
    using FunctionWrapper = std::function<void()>;
    using UniqueLock = std::unique_lock<std::mutex>;
    ThreadPool(size_t numWorkers_ = std::thread::hardware_concurrency()) : m_numWorkers(numWorkers_) {
        m_workers.reserve(numWorkers_);
    }
    void start() {
        for(int i = 0; i < m_numWorkers; i++) {
            m_workers.push_back(std::thread(&ThreadPool::work, this));
        }
    }
    void submit(FunctionWrapper function) {
        UniqueLock lock{m_locker};
        m_taskQueue.push(std::move(function));
        lock.unlock();
        m_cond.notify_one();
    }
    void stop() {
        UniqueLock lock{m_locker};
        m_done = true;
        lock.unlock();
        m_cond.notify_all();
    }
    ~ThreadPool() {
        stop();
        for(auto& worker: m_workers) {
           worker.join();
        }
    }
private:
    size_t m_numWorkers{0};
    bool m_done{false};

    std::vector<std::thread> m_workers;
    std::queue<FunctionWrapper> m_taskQueue;
    
    std::condition_variable m_cond;
    std::mutex m_locker;

    void work() {
        while(true) {
            // poll for task
            UniqueLock lock{m_locker};
            m_cond.wait(lock, [this]() {
                return (!m_taskQueue.empty() || m_done);
            });
            if (m_done) {
                return;
            }
            // get the task
            auto task = std::move(m_taskQueue.front());
            m_taskQueue.pop();
            lock.unlock();
            // execute the task
            task();
        }
    }

};

class Solution {
public:

    std::string getPrefix(const std::string& startUrl) {
        size_t i = 7;
        while(i < startUrl.size() && startUrl[i] != '/') i++;
        return startUrl.substr(0, i);
    }

    vector<string> crawl(string startUrl, HtmlParser htmlParser) {
        ThreadPool pool{8};
        pool.start();

        std::queue<std::string> Urls;
        std::unordered_set<std::string> seen;
        std::vector<std::string> result;
        std::mutex locker;
        std::condition_variable cond;
        int tasksRunning = 0;
        std::string prefix = getPrefix(startUrl);

        Urls.push(startUrl);
        seen.insert(startUrl);
        result.push_back(startUrl);

        while(true) {
            std::unique_lock<std::mutex> lock{locker};
            cond.wait(lock, [&Urls, &tasksRunning]() { return !Urls.empty() || tasksRunning == 0; } );
            while(!Urls.empty()) {
                tasksRunning++;
                auto task = [&seen, 
                             &result, 
                             &Urls, 
                             &locker, 
                             &cond, 
                             &htmlParser, 
                             &prefix, 
                             &tasksRunning,
                             start = std::move(Urls.front())]() {
                                auto matchPrefix = [](const std::string& a, const std::string& b) {
                                                        size_t i = 7;
                                                        while(i < a.size() && i < b.size() && a[i] == b[i]) {
                                                            i++;
                                                        }
                                                        return (i == b.size()) && (i == a.size() || a[i] == '/');
                                                    };
                                std::vector<std::string> ret = htmlParser.getUrls(start);
                                for(auto& url: ret) {
                                    std::unique_lock<std::mutex> lock{locker};
                                    if (seen.find(url) == seen.end() && matchPrefix(url, prefix) ) {
                                        seen.insert(url);
                                        result.push_back(url);
                                        Urls.push(url);
                                        lock.unlock();
                                        cond.notify_one();
                                    }
                                }
                                std::unique_lock<std::mutex> lock{locker};
                                tasksRunning--;
                                if (tasksRunning == 0) {
                                    lock.unlock();
                                    cond.notify_one();
                                }
                            };
                Urls.pop();
                pool.submit(std::move(task));
            }
            if (tasksRunning == 0) {
                break;
            }
        }
        pool.stop();
        return result;
    }
};
