// Implementation of Pokedex per assignment requirements
#include <bits/stdc++.h>
using namespace std;

class BasicException {
protected:
    std::string msg;

public:
    explicit BasicException(const char *_message) : msg(_message ? _message : "") {}
    virtual const char *what() const { return msg.c_str(); }
};

class ArgumentException: public BasicException {
public:
    explicit ArgumentException(const char *m) : BasicException(m) {}
};

class IteratorException: public BasicException {
public:
    explicit IteratorException(const char *m) : BasicException(m) {}
};

struct Pokemon {
    char name[12];
    int id;
};

class Pokedex {
private:
    struct Node {
        Pokemon pm;
        uint32_t typeMask; // up to 32 types, we use 7
        std::vector<int> types; // indices of types
    };

    std::vector<Node> items; // kept sorted by id ascending
    std::string fileName;

    static const std::vector<std::string>& allTypes() {
        static std::vector<std::string> t = {
            "fire", "water", "grass", "electric", "ground", "flying", "dragon"
        };
        return t;
    }

    static int typeIndex(const std::string &s) {
        const auto &t = allTypes();
        for (int i = 0; i < (int)t.size(); ++i) if (t[i] == s) return i;
        return -1;
    }

    // effectiveness[a][b] where a is attack type index, b is defend single type index
    static double eff(int a, int b) {
        // Using a 7-type simplified chart based on canonical interactions
        // Order: fire, water, grass, electric, ground, flying, dragon
        // Initialize neutral
        static double E[7][7];
        static bool inited = false;
        if (!inited) {
            for (int i = 0; i < 7; ++i) for (int j = 0; j < 7; ++j) E[i][j] = 1.0;
            // Fire
            E[0][2] = 2.0;            // fire -> grass
            E[0][0] = 0.5;            // fire -> fire
            E[0][1] = 0.5;            // fire -> water
            E[0][6] = 0.5;            // fire -> dragon
            // Water
            E[1][0] = 2.0;            // water -> fire
            E[1][4] = 2.0;            // water -> ground
            E[1][1] = 0.5;            // water -> water
            E[1][2] = 0.5;            // water -> grass
            E[1][6] = 0.5;            // water -> dragon
            // Grass
            E[2][1] = 2.0;            // grass -> water
            E[2][4] = 2.0;            // grass -> ground
            E[2][0] = 0.5;            // grass -> fire
            E[2][2] = 0.5;            // grass -> grass
            E[2][5] = 0.5;            // grass -> flying
            E[2][6] = 0.5;            // grass -> dragon
            // Electric
            E[3][1] = 2.0;            // electric -> water
            E[3][5] = 2.0;            // electric -> flying
            E[3][4] = 0.0;            // electric -> ground (immune)
            E[3][3] = 0.5;            // electric -> electric
            E[3][2] = 0.5;            // electric -> grass
            E[3][6] = 0.5;            // electric -> dragon
            // Ground
            E[4][0] = 2.0;            // ground -> fire
            E[4][3] = 2.0;            // ground -> electric
            E[4][5] = 0.0;            // ground -> flying (immune)
            E[4][2] = 0.5;            // ground -> grass
            // Flying
            E[5][2] = 2.0;            // flying -> grass
            E[5][3] = 0.5;            // flying -> electric
            // Dragon
            E[6][6] = 2.0;            // dragon -> dragon
            inited = true;
        }
        if (a < 0 || a >= 7 || b < 0 || b >= 7) return 1.0;
        return E[a][b];
    }

    static bool validName(const char *name) {
        if (!name) return false;
        int n = (int)strlen(name);
        if (n <= 0 || n > 10) return false;
        for (int i = 0; i < n; ++i) {
            unsigned char c = (unsigned char)name[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))) return false;
        }
        return true;
    }

    static std::string upperErrorArg(const std::string &what, const std::string &val) {
        return std::string("Argument Error: ") + what + " (" + val + ")";
    }

    void sortById() {
        std::sort(items.begin(), items.end(), [](const Node &a, const Node &b){ return a.pm.id < b.pm.id; });
    }

    void load() {
        items.clear();
        std::ifstream fin(fileName);
        if (!fin.good()) return; // no file yet
        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            std::istringstream iss(line);
            int id; std::string name, typesStr;
            if (!(iss >> id >> name >> typesStr)) continue;
            Node node{};
            node.pm.id = id;
            memset(node.pm.name, 0, sizeof node.pm.name);
            std::strncpy(node.pm.name, name.c_str(), sizeof node.pm.name - 1);
            node.typeMask = 0;
            node.types.clear();
            std::stringstream ss(typesStr);
            std::string tok;
            while (std::getline(ss, tok, '#')) {
                int idx = typeIndex(tok);
                if (idx >= 0) {
                    if (!(node.typeMask & (1u<<idx))) {
                        node.typeMask |= (1u<<idx);
                        node.types.push_back(idx);
                    }
                }
            }
            if (!node.types.empty()) items.push_back(node);
        }
        sortById();
    }

    void save() const {
        std::ofstream fout(fileName, std::ios::trunc);
        if (!fout.good()) return;
        for (auto &n : items) {
            std::string typesStr;
            bool first = true;
            for (int ti : n.types) {
                if (!first) typesStr.push_back('#');
                first = false;
                typesStr += allTypes()[ti];
            }
            fout << n.pm.id << ' ' << n.pm.name << ' ' << typesStr << '\n';
        }
    }

    int findIndexById(int id) const {
        int l = 0, r = (int)items.size()-1;
        while (l <= r) {
            int m = (l+r)/2;
            if (items[m].pm.id == id) return m;
            if (items[m].pm.id < id) l = m+1; else r = m-1;
        }
        return -1;
    }

public:
    explicit Pokedex(const char *_fileName) {
        fileName = _fileName ? _fileName : "pokedex.dat";
        load();
    }

    ~Pokedex() {
        save();
    }

    bool pokeAdd(const char *name, int id, const char *types) {
        // validate arguments first; if invalid, throw and do nothing
        if (!validName(name)) {
            throw ArgumentException(upperErrorArg("PM Name Invalid", name ? std::string(name) : std::string("")).c_str());
        }
        if (id <= 0) {
            std::string idstr = std::to_string(id);
            throw ArgumentException(upperErrorArg("PM ID Invalid", idstr).c_str());
        }
        if (!types) {
            throw ArgumentException(upperErrorArg("PM Type Invalid", std::string("")).c_str());
        }
        std::stringstream ss(types);
        std::string tok;
        uint32_t mask = 0;
        std::vector<int> tvec;
        while (std::getline(ss, tok, '#')) {
            if (tok.empty()) continue;
            int idx = typeIndex(tok);
            if (idx < 0) {
                throw ArgumentException(upperErrorArg("PM Type Invalid", tok).c_str());
            }
            if (!(mask & (1u<<idx))) { mask |= (1u<<idx); tvec.push_back(idx); }
            if ((int)tvec.size() > 7) break; // safety
        }
        if (tvec.empty()) {
            throw ArgumentException(upperErrorArg("PM Type Invalid", std::string("")).c_str());
        }

        if (findIndexById(id) != -1) return false; // already exists
        // Name uniqueness: ensure no duplicates
        for (auto &n : items) if (strcmp(n.pm.name, name) == 0) return false;

        Node node{};
        node.pm.id = id;
        memset(node.pm.name, 0, sizeof node.pm.name);
        std::strncpy(node.pm.name, name, sizeof node.pm.name - 1);
        node.typeMask = mask;
        node.types = tvec;
        items.push_back(node);
        sortById();
        return true;
    }

    bool pokeDel(int id) {
        int idx = findIndexById(id);
        if (idx == -1) return false;
        items.erase(items.begin() + idx);
        return true;
    }

    std::string pokeFind(int id) const {
        int idx = findIndexById(id);
        if (idx == -1) return std::string("None");
        return std::string(items[idx].pm.name);
    }

    std::string typeFind(const char *types) const {
        if (!types) {
            throw ArgumentException(upperErrorArg("PM Type Invalid", std::string("")).c_str());
        }
        std::stringstream ss(types);
        std::string tok;
        uint32_t need = 0;
        while (std::getline(ss, tok, '#')) {
            if (tok.empty()) continue;
            int idx = typeIndex(tok);
            if (idx < 0) {
                throw ArgumentException(upperErrorArg("PM Type Invalid", tok).c_str());
            }
            need |= (1u<<idx);
        }
        if (need == 0) {
            throw ArgumentException(upperErrorArg("PM Type Invalid", std::string("")).c_str());
        }
        std::vector<std::string> names;
        for (auto &n : items) {
            if ( (n.typeMask & need) == need ) {
                names.push_back(n.pm.name);
            }
        }
        if (names.empty()) return std::string("None");
        std::ostringstream out;
        out << names.size() << '\n';
        for (auto &nm : names) out << nm << '\n';
        std::string s = out.str();
        if (!s.empty() && s.back() == '\n') s.pop_back();
        return s;
    }

    float attack(const char *type, int id) const {
        int idx = findIndexById(id);
        if (idx == -1) return -1.0f;
        int atk = typeIndex(type ? std::string(type) : std::string(""));
        if (atk < 0) return -1.0f; // should not happen per spec
        double mul = 1.0;
        for (int t : items[idx].types) mul *= eff(atk, t);
        return (float)mul;
    }

    int catchTry() const {
        if (items.empty()) return 0;
        std::vector<char> owned(items.size(), 0);
        std::queue<size_t> q;
        // own the smallest id (index 0 since sorted ascending)
        owned[0] = 1;
        q.push(0);
        int ownedCount = 1;
        auto canCapture = [&](const Node &my, const Node &wild)->bool{
            for (int atk : my.types) {
                double mul = 1.0;
                for (int def : wild.types) mul *= eff(atk, def);
                if (mul >= 2.0 - 1e-9) return true;
            }
            return false;
        };
        while (!q.empty()) {
            size_t j = q.front(); q.pop();
            for (size_t i = 0; i < items.size(); ++i) {
                if (owned[i]) continue;
                if (canCapture(items[j], items[i])) {
                    owned[i] = 1; ++ownedCount; q.push(i);
                }
            }
        }
        return ownedCount;
    }

    struct iterator {
        const Pokedex *owner = nullptr;
        size_t index = 0; // 0..owner->items.size()

        iterator() = default;
        iterator(const Pokedex *o, size_t i): owner(o), index(i) {}

        iterator &operator++() {
            if (!owner) throw IteratorException("Iterator Error: Invalid");
            if (index >= owner->items.size()) throw IteratorException("Iterator Error: Invalid");
            ++index; return *this;
        }
        iterator &operator--() {
            if (!owner) throw IteratorException("Iterator Error: Invalid");
            if (index == 0) throw IteratorException("Iterator Error: Invalid");
            --index; return *this;
        }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }
        iterator operator--(int) { iterator tmp = *this; --(*this); return tmp; }
        iterator & operator = (const iterator &rhs) { owner = rhs.owner; index = rhs.index; return *this; }
        bool operator == (const iterator &rhs) const { return owner == rhs.owner && index == rhs.index; }
        bool operator != (const iterator &rhs) const { return !(*this == rhs); }
        Pokemon & operator*() const {
            if (!owner || index >= owner->items.size()) throw IteratorException("Iterator Error: Invalid");
            // discard constness to return reference as required; underlying storage is mutable in class
            return const_cast<Pokemon&>(owner->items[index].pm);
        }
        Pokemon *operator->() const {
            if (!owner || index >= owner->items.size()) throw IteratorException("Iterator Error: Invalid");
            return const_cast<Pokemon*>(&owner->items[index].pm);
        }
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, items.size()); }
};
