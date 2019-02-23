
#include <iostream>
#include <optional>
#include <random>
#include <vector>

class FixedSet;

class HashFunction {
public:
    HashFunction() : arg_a(0), arg_b(0), modulus(1) {}

    HashFunction(int64_t arg_one, int64_t arg_two, int64_t mod)
        : arg_a(arg_one), arg_b(arg_two), modulus(mod) {}

    int64_t operator()(int64_t number) const {
        int64_t result;
        result = (arg_a * number) % modulus;
        result = (result + arg_b) % modulus;
        if (result < 0) {
            result += modulus;
        }
        return result;
    }


    static HashFunction GenerateRandom(std::mt19937* generator, int64_t mod) {

        std::uniform_int_distribution<int> dist(0, mod - 1);
        int arg_one = dist(*generator);
        int arg_two = dist(*generator);
        return HashFunction(arg_one, arg_two, mod);
    }

    int64_t arg_a;
    int64_t arg_b;
    int64_t modulus;
    static constexpr int64_t kPrime = 2'147'483'647;
};

class InternalHashTable {
public:
    InternalHashTable(): hash_function_() {}

    void Initialize(const std::vector<int>& elements, std::mt19937* generator);
    bool Contains(int value) const;
private:
    HashFunction hash_function_;
    bool HasCollisions(const std::vector<int>& elements);
    void BuildHashTable(const std::vector<int>& elements);
    std::vector<std::optional<int>> info_;
};

class FixedSet {
public:
    void Initialize(const std::vector<int>& elements);
    bool Contains(int value) const;

    int64_t additional_memory;
private:
    HashFunction FindHashFunction(const std::vector<int>& elements, std::mt19937* generator);
    void BuildHashTables(const std::vector<int>& elements, std::mt19937* generator);

    static std::vector<int> ComputeCollisions(const std::vector<int>& elements,
                                        HashFunction hash_func, int64_t modulus);
    std::vector<std::vector<int>> BuildBackets(const std::vector<int>& elements);
    int64_t ComputeMemory(const std::vector<int>& backet_sizes);

    HashFunction hash_function_;
    std::vector<InternalHashTable> hash_tables_;

    friend InternalHashTable;
};

std::vector<std::vector<int>> FixedSet::BuildBackets(const std::vector<int>& elements) {

    std::vector<std::vector<int>> backets;
    int backets_num = elements.size();
    backets.resize(backets_num);
    for (const auto& elem : elements) {
        int index = hash_function_(elem) % backets_num;
        (backets)[index].push_back(elem);
    }
    return backets;
}

int64_t FixedSet::ComputeMemory(const std::vector<int>& backet_sizes) {

    additional_memory = 0;
    for (const auto& elem : backet_sizes) {
        additional_memory += elem * static_cast<int64_t>(elem);
    }
    return additional_memory;
}

std::vector<int> FixedSet::ComputeCollisions(const std::vector<int>& elements,
                                              HashFunction hash_func, int64_t modulus) {

    std::vector<int> backet_sizes(modulus, 0);

    for (const auto& elem : elements) {
        int index = hash_func(elem) % modulus;
        ++backet_sizes[index];
    }

    return backet_sizes;
}

HashFunction FixedSet::FindHashFunction(const std::vector<int>& elements, std::mt19937* generator) {
    std::vector<int> backet_sizes;

    HashFunction hash_func;

    int64_t additional_memory;
    do {
        hash_func = HashFunction::GenerateRandom(generator, HashFunction::kPrime);

        backet_sizes = ComputeCollisions(elements, hash_func,
                                         static_cast<int64_t>(elements.size()));

        additional_memory = ComputeMemory(backet_sizes);
    } while (static_cast<uint64_t>(additional_memory) > 4 * elements.size());
    return hash_func;
}

void FixedSet::BuildHashTables(const std::vector<int>& elements, std::mt19937* generator) {

    std::vector<std::vector<int>> backets = BuildBackets(elements);

    hash_tables_.reserve(elements.size());
    for (size_t index = 0; index < elements.size(); ++index) {

        InternalHashTable table;
        table.Initialize(backets[index], generator);

        hash_tables_.emplace_back(std::move(table));
    }
}

void FixedSet::Initialize(const std::vector<int>& elements) {

    if (!hash_tables_.empty()) {
        hash_tables_.clear();
    }

    std::mt19937 generator(333);

    hash_function_ = FindHashFunction(elements, &generator);

    BuildHashTables(elements, &generator);
}

bool FixedSet::Contains(int value) const {
    if (hash_tables_.empty()) {
        return false;
    }

    int index = hash_function_(value) % hash_tables_.size();

    return hash_tables_[index].Contains(value);
}

bool InternalHashTable::Contains(int value) const {
    if (info_.empty()) {
        return false;
    }

    int index = hash_function_(value) % info_.size();
    return info_[index].has_value() && info_[index].value() == value;
}

bool InternalHashTable::HasCollisions(const std::vector<int>& elements) {

    int64_t modulus = elements.size() * elements.size();
    std::vector<int> collisions;
    collisions = FixedSet::ComputeCollisions(elements, hash_function_, modulus);
    for (const auto& elem: collisions) {
        if (elem > 1) {
            return true;
        }
    }
    return false;
}

void InternalHashTable::BuildHashTable(const std::vector<int>& elements) {
    for (const auto& elem : elements) {
        size_t index = hash_function_(elem) % info_.size();
        info_[index] = elem;
    }
}

void InternalHashTable::Initialize(const std::vector<int>& elements, std::mt19937* generator) {

    size_t size = elements.size() * elements.size();

    info_.resize(size);
    HashFunction hash_func;

    do {
        hash_func = HashFunction::GenerateRandom(generator, HashFunction::kPrime);
        hash_function_ = hash_func;
    } while (HasCollisions(elements));

    BuildHashTable(elements);
}
