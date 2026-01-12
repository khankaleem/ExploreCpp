/**
 * // This is the HtmlParser's API interface.
 * // You should not implement it, or speculate about its implementation
 * class HtmlParser {
 *   public:
 *     vector<string> getUrls(string url);
 * };
 */
class Solution {
public:
    inline bool matchPrefix(const std::string& a, const std::string& b) {
        size_t i = 7;
        while(i < a.size() && i < b.size() && a[i] == b[i]) {
            i++;
        }
        return (i == b.size()) && (i == a.size() || a[i] == '/');
    }
    std::string getPrefix(const std::string& startUrl) {
        size_t i = 7;
        while(i < startUrl.size() && startUrl[i] != '/') i++;
        return startUrl.substr(0, i);
    }

    vector<string> crawl(string startUrl, HtmlParser htmlParser) {
        // Can tune this : For IO Bound we can keep this high, for compute based it vcan be kept low
        auto n_par = std::thread::hardware_concurrency() ;

        std::vector<std::string> res;
        
        std::queue< std::future< std::vector<std::string>  > > workers;
        queue<std::string> items;
        std::unordered_set<std::string> visited;
        items.push(startUrl);
        visited.insert(startUrl);
        res.push_back(startUrl);
        std::string prefix = getPrefix(startUrl);

        while(!(workers.empty() && items.empty())) {
            
            // Poll worker threads for completion
            int nw = workers.size();
            for(int i = 0; i < nw; i++) {
                auto status = workers.front().wait_for(0s);
                if (status != std::future_status::ready) {
                    workers.push(std::move(workers.front()));
                }
                else {
                    auto ret = workers.front().get();
                    for(const auto& item: ret) {
                        if (visited.find(item) == visited.end()
                            &&
                            matchPrefix(item, prefix)
                           ) {
                            res.push_back(item);
                            visited.insert(item);
                            items.push(std::move(item));
                        }
                    }
                }

                workers.pop();
            }

            // Create task for available url
            while(workers.size() < n_par && !items.empty()) {
                workers.push(std::async(
                                            std::launch::async,
                                            [&htmlParser, item = std::move(items.front())]() {
                                                return htmlParser.getUrls(item);
                                            }
                ));
                items.pop();
            }
   
            // For fast polling remove this, for slow polling and less CPU consumption can tune
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return res;
    }
};
