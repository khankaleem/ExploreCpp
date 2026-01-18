class TimeMap {
    using Node = std::pair<int, std::string>;
    using Container = std::vector<Node>;
    using Map = std::unordered_map<std::string, Container>;
    Map m_map;

public:

    TimeMap() {
        m_map.max_load_factor(0.72);
    }
    
    void set(string key, string value, int timestamp) {
        auto [it, _] = m_map.try_emplace(std::move(key));
        it->second.emplace_back(timestamp, std::move(value));
    }
    
    string get(string key, int timestamp) {
        auto it = m_map.find(key);
        if (it == m_map.end()) [[unlikely]] {
            return "";
        }
        auto searchIterator = 
                std::upper_bound(it->second.begin(), it->second.end(), timestamp, 
                    [](int value, const Node& hay) {
                        return value < hay.first;
                    }
                );
        return searchIterator == it->second.begin() ? "" : std::prev(searchIterator)->second;
    }
};

/**
 * Your TimeMap object will be instantiated and called as such:
 * TimeMap* obj = new TimeMap();
 * obj->set(key,value,timestamp);
 * string param_2 = obj->get(key,timestamp);
 */
