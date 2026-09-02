#pragma once

namespace yexian {

template <typename Key, typename Value>
class CachePolicy
{
public:
    virtual ~CachePolicy() {};
    // 添加缓存接口
    virtual void put(Key key, Value value) = 0;
    // key是传入参数，访问到的值以传出参数的形式返回：成功返回true
    virtual bool get(Key key, Value& value) = 0;
    // 如果缓存能找到key，直接返回value
    virtual Value get(Key key) = 0;
};

} // namespace yexian