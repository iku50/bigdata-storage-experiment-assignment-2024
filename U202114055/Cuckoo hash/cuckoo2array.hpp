#ifndef __CUCKOO2ARRAY_HPP__
#define __CUCKOO2ARRAY_HPP__

#include <stdio.h>
#include <ctime> // for std::time
#include <vector>
#include <random>
#include <functional> // for std::hash

template<typename K, typename V>
class CUCKOO2 {
    private:
        int32_t capacity;
        int32_t max_depth;
        uint32_t Size;
        uint32_t seed;
        std::default_random_engine generator; // 随机数生成器
        std::uniform_int_distribution<int32_t> distribution; // 分布对象
        std::vector<std::pair<K, V>> hashTable1;
        std::vector<int8_t> bitset1; // bitset 不能动态设置大小，vector<bool>有缺陷
        std::vector<std::pair<K, V>> hashTable2;
        std::vector<int8_t> bitset2;
        
        int32_t seize_place(int8_t tableIndex, int32_t index, int32_t depth)  {
            if (depth >= max_depth) {
                return 0;
            }
            auto my_hash = (tableIndex ^ 1) ? &CUCKOO2::my_hash2 : &CUCKOO2::my_hash1;
            std::vector<int8_t>& nextBitset = (tableIndex ^ 1) ? bitset2 : bitset1;
            std::vector<std::pair<K,V>>& hashTable = tableIndex ? hashTable2 : hashTable1;
            std::vector<std::pair<K,V>>& nextHashTable = (tableIndex ^ 1) ? hashTable2 : hashTable1;
            K& now_key = hashTable[index].first; 
            int32_t nextIndex = (this->*my_hash)(now_key);
            int32_t ret;
            if (!nextBitset[nextIndex]) {
                ret = depth;
            }
            else if (!(ret = seize_place(tableIndex ^ 1, nextIndex, depth + 1))) {
                return 0;
            }
            nextBitset[nextIndex] = true;
            nextHashTable[nextIndex] = hashTable[index];
            return ret;
        }

        int32_t my_hash1(const K &key) {
            return std::hash<K>{}(key) % capacity;
        }

        int32_t my_hash2(const K &key)  {
            return (1ll * std::hash<K>{}(key) * 19260817 + 10007) % capacity;
        }
        // void expand_capacity() { //扩容之后 capacity 就不是质数了，碰撞概率大大增加
        //     int32_t preCapacity = capacity;
        //     capacity <<= 1;
        //     std::vector<std::pair<K, V>> preHashTable1(capacity);
        //     std::vector<int8_t> preBitset1(capacity); // bitset 不能动态设置大小，vector<bool>有缺陷
        //     std::vector<std::pair<K, V>> preHashTable2(capacity);
        //     std::vector<int8_t> preBitset2(capacity);
        //     hashTable1.swap(preHashTable1);
        //     bitset1.swap(preBitset1);
        //     hashTable2.swap(preHashTable2);
        //     bitset2.swap(preBitset2);

        //     for(int32_t i = 0; i < preCapacity;++ i) {
        //         if (preBitset1[i]) {
        //             insert()
        //         }
        //         if (preBitset2[i]) {
        //             insert()
        //         }
        //     }
        // }
    public:
        /*
        @param capacity 设置hashMap的容量
        @param max_depth 最大搜索深度
        @param seedValue 随机种子
        */
        CUCKOO2(int32_t capacity, int32_t max_depth = 10, uint32_t seedValue = 0)
                        : capacity(capacity), max_depth(max_depth), Size(0)
                        , bitset1(capacity), bitset2(capacity), seed(seedValue) {
            if (!seed) {
                // 如果种子为0，则使用当前时间作为种子
                seed = static_cast<uint32_t>(std::time(nullptr));
            } 
            generator.seed(seed); // 初始化随机数生成器
            distribution = std::uniform_int_distribution<int>(0, 1); // 初始化分布对象
            hashTable1.resize(capacity);
            hashTable2.resize(capacity);
        }
        /*
        @param key 需要插入的数据的索引
        @param data 需要存储的数据
        @return 是否插入成功 1成功，0失败
        */
        bool insert(const K &key, const V &data) {
            int32_t index1 = my_hash1(key);
            int32_t index2 = my_hash2(key);
            bool free1 = !bitset1[index1];
            bool free2 = !bitset2[index2];
            bool random_num = distribution(generator); // 随机找一个插入
            int32_t index = random_num ? index2 : index1;
            std::vector<int8_t>& bitset = random_num ? bitset2 : bitset1;
            std::vector<std::pair<K,V>>& hashTable = random_num ? hashTable2 : hashTable1;

            if (!free1 && !free2) {
                int32_t depth = 0;
                if (!(depth = seize_place(random_num, index, 1))) {
                    random_num ^= 1; // 随机的插入遇到死循环了，试试另一边
                    index = random_num ? index2 : index1;
                    bitset = random_num ? bitset2 : bitset1;
                    hashTable = random_num ? hashTable2 : hashTable1;
                    if (!(depth = seize_place(random_num, index, 1))) {
                        // 都不行，报错
                        // expand_capacity();
                        #ifdef __CUCKOO2_DEBUG__
                            printf("Failed to seize place.\n");
                        #endif
                        return false;
                    }
                }
                #ifdef __CUCKOO2_DEBUG__
                    printf("Seized place for %d cycles.\n", depth);
                #endif
            }
            else if (free1 && !free2) {
                index = index1;
                bitset = bitset1;
                hashTable = hashTable1;
            }
            else if (free2 && !free1) {
                index = index2;
                bitset = bitset2;
                hashTable = hashTable2;
            }

            bitset[index] = true;
            hashTable[index] = std::pair<K, V>(key, data);
            Size ++;
            return true;
        }
        /*
        @param key 需要移除的数据的索引
        */
        void remove(const K &key)  {
            int32_t index1 = my_hash1(key);
            if (bitset1[index1] && hashTable1[index1].first == key) {
                bitset1[index1] = 0;
                Size --;
                return;
            }
            int32_t index2 = my_hash2(key);
            if (bitset2[index2] && hashTable2[index2].first == key) {
                bitset2[index2] = 0;
                Size --;
                return;
            }
        }
        /*
        @param key 需要查找是否存在的索引
        @return 1表示存在，0不存在
        */
        bool find(const K &key)  {
            int32_t index1 = my_hash1(key);
            if (bitset1[index1] && hashTable1[index1].first == key) {
                return true;
            }
            int32_t index2 = my_hash2(key);
            if (bitset1[index2] && hashTable1[index2].first == key) {
                return true;
            }
            return false;
        }

        inline uint32_t size() {
            return Size;
        }
        inline uint32_t get_seed() {
            return seed;
        }
};

#endif