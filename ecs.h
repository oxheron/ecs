#pragma once

// external
#include "stduuid/uuid.h"
#include "robin-hood/robin_hood.h"
#include "good-random/good-random.h"

#include <cstdint>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <typeindex>

// This object is not guaranteed to stay stable
// DO NOT STORE
template <typename... Args>
struct ECSView
{
    std::vector<std::tuple<uuids::uuid, std::decay_t<Args>*...>> view_obj;

    void and_or_exclude(const robin_hood::unordered_set<uuids::uuid>& uuids, 
        bool and_if_true = true)
    {
        size_t current_size = view_obj.size();

        for (size_t i = 0; i < current_size; )
        {
            if (and_if_true != uuids.contains(std::get<uuids::uuid>(view_obj[i])))
            {
                std::swap(view_obj[i], view_obj.back());
                view_obj.pop_back(); 
            }
        }
    }

    void execute(void (*fn)(Args*...))
    {
        std::function std_fn = fn;
        execute(std_fn);
    }

    void execute(const std::function<void(uuids::uuid, Args*...)>& callback)
    {
        for (auto k : view_obj)
        {
            std::apply(callback, k);
        }
    }
};

class IECSPool
{
public:
    virtual void remove_entity(uuids::uuid) = 0;
    virtual size_t count() = 0; 
};

template <typename T>
class ECSPool : public IECSPool
{
private:
    using component_type = std::decay_t<T>;
    using component_ref = component_type&;
    using component_ptr = component_type*;
     
    robin_hood::unordered_flat_map<uuids::uuid, size_t> entity_indices;
    std::vector<uuids::uuid> entity_list;
    std::vector<component_type> component_list; 


public: 

    void add_entity(uuids::uuid uuid, T&& entity)
    {
        entity_indices.insert({uuid, component_list.size()});
        entity_list.emplace_back(uuid);
        component_list.emplace_back(std::forward<T>(entity));
    }

    void remove_entity(uuids::uuid uuid) override
    {
        entity_indices.erase(uuid);
        std::swap(component_list[entity_indices[uuid]], component_list.back());
        component_list.pop_back();
        std::swap(entity_list[entity_indices[uuid]], entity_list.back());
        entity_list.pop_back();
    }

    size_t count() override { return component_list.size(); } 

    template <typename... Args>
    void add_to_view(ECSView<Args...>& view)
    {
        if (view.view_obj.size() == 0)
        {
            for (size_t i = 0; i < component_list.size(); i++)
            {
                std::tuple<uuids::uuid, std::decay_t<Args>*...> tuple;
                std::get<component_ptr>(tuple) = &component_list[i];
                std::get<uuids::uuid>(tuple) = entity_list[i];
                view.view_obj.emplace_back(tuple);
            }
        }
        else
        {
            size_t current_size = view.view_obj.size();

            for (size_t i = 0; i < current_size; )
            {
                if (!entity_indices.contains(std::get<uuids::uuid>(
                    view.view_obj[i])))
                {
                    std::swap(view.view_obj[i], view.view_obj.back());
                    view.view_obj.pop_back(); 
                }
                else
                {
                    std::get<component_ptr>(view.view_obj[i]) = 
                        &component_list[i];
                    i++;
                }
            }
        }
    }
};

class ECSManager
{
private:
    robin_hood::unordered_map<std::type_index, IECSPool*> pools; // Each pool holds its own component type
    std::mt19937 rng;
    uuids::basic_uuid_random_generator<std::mt19937> generator;

public:
    inline ECSManager()
        : rng(setup_mt()), generator(&rng)
    {}

    ECSManager(ECSManager& other) = delete;
    ECSManager& operator=(ECSManager& other) = delete;

    uuids::uuid gen_uuid() 
    {
        return generator();
    }

    // Entity managment
    inline void remove_entity(uuids::uuid uuid)
    {
        for (auto [k, v] : pools)
        {
            v->remove_entity(uuid);
        }
    }

    template <typename T>
    void add_component(uuids::uuid uuid, T&& entity)
    {
        get_create_typed_pool<T>()->add_entity(uuid, std::forward<T>(entity));
    }

    template <typename T>
    void remove_component(uuids::uuid uuid) 
    {
        pools[std::type_index(typeid(T))]->remove_entity(uuid);
    }


    template <typename... Args>
    ECSView<Args...> get_view()
    {
        ECSView<Args...> view;

        std::type_index smallest_tid = gen_smallest_tid<Args...>();

        // Call shortest
        ((void)add_to_view_smallest<Args, Args...>(view, 
            smallest_tid), ...);

        // Call the rest
        ((void)add_to_view<Args, Args...>(view, smallest_tid), ...);

        return view;
    }

private:
    template <typename T>
    inline ECSPool<T>* get_create_typed_pool() 
    { 
        if (!pools.contains(std::type_index(typeid(T))))
        {
            pools[std::type_index(typeid(T))] = new ECSPool<T>;
        }
        return (ECSPool<T>*) pools[std::type_index(typeid(T))];
    }

    template <typename... Args>
    std::type_index gen_smallest_tid()
    {
        // Find shortest w/ interface fn
        std::array<std::type_index, sizeof...(Args)> tids =
        {
            std::type_index(typeid(Args)) ...
        };

        size_t smallest_size = 0;
        std::type_index smallest_tid = typeid(nullptr);

        for (size_t i = 0; i < tids.size(); i++)
        {
            if (pools[tids[i]]->count() < smallest_size) 
            {
                smallest_size = pools[tids[i]]->count();
                smallest_tid = tids[i];
            }
        }

        return smallest_tid;
    }

    template <typename T, typename... Args> 
    void add_to_view(ECSView<Args...>& view, std::type_index smallest_tid)
    {
        if (smallest_tid == std::type_index(typeid(T))) return;       
        auto* pool = get_create_typed_pool<T>();
        pool->add_to_view(view);
    }

    template <typename T, typename... Args> 
    void add_to_view_smallest(ECSView<Args...>& view, 
        std::type_index smallest_tid)
    {
        if (smallest_tid != std::type_index(typeid(T))) return;       
        auto* pool = get_create_typed_pool<T>();
        pool->add_to_view(view);
    }
};
