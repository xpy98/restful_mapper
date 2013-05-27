#ifndef RESTFUL_MAPPER_MODEL_H
#define RESTFUL_MAPPER_MODEL_H

#include <restful_mapper/api.h>
#include <restful_mapper/mapper.h>
#include <restful_mapper/query.h>

namespace restful_mapper
{

template <class T>
class Model
{
public:
  typedef ModelCollection<T> Collection;

  Model() : exists_(false) {}

  virtual void map_set(Mapper &mapper) const
  {
    throw std::logic_error(std::string("map_set not implemented for ") + typeid(T).name());
  }

  virtual void map_get(const Mapper &mapper)
  {
    throw std::logic_error(std::string("map_get not implemented for ") + typeid(T).name());
  }

  virtual std::string endpoint() const
  {
    throw std::logic_error(std::string("endpoint not implemented for ") + typeid(T).name());
  }

  virtual const Primary &primary() const
  {
    throw std::logic_error(std::string("primary not implemented for ") + typeid(T).name());
  }

  void from_json(std::string values, const int &flags = 0)
  {
    Mapper mapper(values, flags);
    map_get(mapper);

    exists_ = true;
  }

  std::string to_json(const int &flags = 0) const
  {
    Mapper mapper(flags);
    map_set(mapper);

    return mapper.dump();
  }

  std::string read_field(const std::string &field) const
  {
    Mapper mapper(OUTPUT_SINGLE_FIELD | KEEP_FIELDS_DIRTY | IGNORE_DIRTY_FLAG);
    mapper.set_field_filter(field);
    map_set(mapper);

    return mapper.dump();
  }

  bool is_dirty() const
  {
    return to_json(KEEP_FIELDS_DIRTY).size() > 2;
  }

  void reload()
  {
    if (exists())
    {
      from_json(Api::get(url()));
    }
  }

  void destroy()
  {
    if (exists())
    {
      Api::del(url());

      // Reload all attributes
      from_json(to_json(IGNORE_DIRTY_FLAG), TOUCH_FIELDS | IGNORE_MISSING_FIELDS);

      // Reset and clear primary
      const_cast<Primary &>(primary()) = Primary();
      const_cast<Primary &>(primary()).clear();

      exists_ = false;
    }
  }

  void save()
  {
    if (exists())
    {
      from_json(Api::put(url(), to_json()), IGNORE_MISSING_FIELDS);
    }
    else
    {
      from_json(Api::post(url(), to_json()), IGNORE_MISSING_FIELDS);
    }
  }

  virtual T clone() const
  {
    T cloned;

    cloned.from_json(to_json(KEEP_FIELDS_DIRTY | IGNORE_DIRTY_FLAG), TOUCH_FIELDS | IGNORE_MISSING_FIELDS);

    return cloned;
  }

  void reload_one(const std::string &relationship)
  {
    if (exists())
    {
      Json::Emitter emitter;

      emitter.emit_map_open();
      emitter.emit_json(relationship, Api::get(url(relationship)));
      emitter.emit_map_close();

      from_json(emitter.dump(), IGNORE_MISSING_FIELDS);
    }
  }

  void reload_many(const std::string &relationship)
  {
    if (exists())
    {
      Json::Parser parser(Api::get(url(relationship)));

      Json::Emitter emitter;

      emitter.emit_map_open();
      emitter.emit(relationship);
      emitter.emit_tree(parser.find("objects").json_tree_ptr());
      emitter.emit_map_close();

      from_json(emitter.dump(), IGNORE_MISSING_FIELDS);
    }
  }

  static T find(const int &id)
  {
    T instance;
    const_cast<Primary &>(instance.primary()).set(id, true);

    instance.exists_ = true;
    instance.reload();

    return instance;
  }

  static Collection find_all()
  {
    Collection objects;

    Json::Parser collector(Api::get(T().url()));

    std::vector<std::string> partials = collector.find("objects").dump_array();
    std::vector<std::string>::const_iterator i, i_end = partials.end();

    for (i = partials.begin(); i != i_end; ++i)
    {
      T instance;
      instance.from_json(*i);

      objects.push_back(instance);
    }

    return objects;
  }

  static T find(Query &query)
  {
    T instance;

    std::string url = Api::query_param(instance.url(), "q", query.single().dump());
    instance.from_json(Api::get(url));

    return instance;
  }

  static Collection find_all(Query &query)
  {
    Collection objects;

    std::string url = Api::query_param(T().url(), "q", query.dump());
    Json::Parser collector(Api::get(url));

    std::vector<std::string> partials = collector.find("objects").dump_array();
    std::vector<std::string>::const_iterator i, i_end = partials.end();

    for (i = partials.begin(); i != i_end; ++i)
    {
      T instance;
      instance.from_json(*i);

      objects.push_back(instance);
    }

    return objects;
  }

  std::string url(std::string nested_endpoint = "") const
  {
    if (exists())
    {
      if (!nested_endpoint.empty())
      {
        nested_endpoint = "/" + nested_endpoint;
      }

      return endpoint() + "/" + std::string(primary()) + nested_endpoint;
    }
    else
    {
      return endpoint();
    }
  }

  const bool &exists() const
  {
    return exists_;
  }

  bool operator==(const T &other) const
  {
    return to_json(KEEP_FIELDS_DIRTY | IGNORE_DIRTY_FLAG) ==
      other.to_json(KEEP_FIELDS_DIRTY | IGNORE_DIRTY_FLAG);
  }

  bool operator!=(const T &other) const
  {
    return !(*this == other);
  }

protected:
  bool exists_;
};

}

#endif // RESTFUL_MAPPER_MODEL_H

