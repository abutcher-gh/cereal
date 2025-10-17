//---------------------------------------------------------------
// Trial for native_json built-in support for optional types.
//---------------------------------------------------------------
// Standalone optionals are rendered wrapped in an array.
//   [value] if a value is set
//   [] if no value is set
//
// Objects (the more common case) simply render optional property
// values as normal and exclude the properties that are not set.
//
// No special serialization logic is required in the client; just
// serialize the fields as if they were not optional.
//
// Since support for optionality is built into the archives,
// specific serializers for the optional types should NOT be
// included in the translation unit.
//
// For custom optional types, this means if you only use
// native_json serialization, you don't even need to write a
// serializer for the type.
//---------------------------------------------------------------


// Simple custom optional with different API to std.optional.
//
// NOTE: No serialize/load/save functions are required for this (or
//       std.optional) when using NativeJSON*Archives.
//
template <class T>
class Property
{
public:
  Property() : isSet(false) {}
  bool IsSet() const { return isSet; }
  const T Get() const { return value; }
  void Set(const T& value) { this->value = value; isSet = true; }

private:
  bool isSet;
  T value;
};


#include <optional>
#include <cereal/cereal.hpp>

struct X
{
   std::optional<int> a = {};
   Property<int> b = {};

   template <typename Archive>
   void serialize(Archive& ar) {
      ar & CEREAL_NVP(a)
         & CEREAL_NVP(b)
         ;
   }
};

struct Y
{
   std::optional<X> x1;
   Property<X> x2;
   X mandatory = {.a = 3};

   template <typename Archive>
   void serialize(Archive& ar) {
      ar & CEREAL_NVP(mandatory)
         & CEREAL_NVP(x1)
         & CEREAL_NVP(x2)
         & CEREAL_NVP(mandatory)
         & CEREAL_NVP(x2)
         ;
   }
};


#include <cereal/archives/native_json.hpp>
using OutputArchive = cereal::NativeJSONOutputArchive;
using InputArchive = cereal::NativeJSONInputArchive;

namespace cereal
{
   template <typename T>
   struct NativeJSONOptional<Property<T>>
   {
      static bool has_value(Property<T> const& prop) { return prop.IsSet(); }
      static decltype(auto) value(Property<T> const& prop) { return prop.Get(); }
      static void value(Property<T>& prop, T const& v) { prop.Set(v); }
   };
}


#include <iostream>

void dump(X const& x)
{
   std::cerr << "{ a: " << (x.a.value_or(-7777)) << ", b: " << (x.b.IsSet()? x.b.Get() : -5555) << " }\n";
}

void dump(Y const& y)
{
   std::cerr << "x1: "; if (y.x1) dump(*y.x1); else std::cerr << "None\n";
   std::cerr << "x2: "; if (y.x2.IsSet()) dump(y.x2.Get()); else std::cerr << "None\n";
   std::cerr << "m: "; dump(y.mandatory);
}


#include <cereal/types/vector.hpp>

int main()
{

   X x;
   X y {.a = 3};
   X z; z.b.Set(8);
   X w {.a = 10}; w.b.Set(20);

   #if !USE_ORIGINAL_CEREAL_JSON_ARCHIVE
   std::cerr << cereal::traits::is_optional<std::optional<int>>::value << "\n";
   std::cerr << cereal::traits::is_optional<int>::value << "\n";
   std::cerr << cereal::traits::is_optional<Property<int>>::value << "\n";
   std::cerr << "-------------------\n";
   std::cerr << x.a.has_value() << "\n";
   std::cerr << x.b.IsSet() << "\n";
   std::cerr << y.a.has_value() << "\n";
   std::cerr << y.b.IsSet() << "\n";
   std::cerr << z.a.has_value() << "\n";
   std::cerr << z.b.IsSet() << "\n";
   std::cerr << w.a.has_value() << "\n";
   std::cerr << w.b.IsSet() << "\n";
   std::cerr << "-------------------\n";

   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(x.a)>>::has_value(x.a) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(x.b)>>::has_value(x.b) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(y.a)>>::has_value(y.a) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(y.b)>>::has_value(y.b) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(z.a)>>::has_value(z.a) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(z.b)>>::has_value(z.b) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(w.a)>>::has_value(w.a) << "\n";
   std::cerr << cereal::NativeJSONOptional<std::decay_t<decltype(w.b)>>::has_value(w.b) << "\n";
   std::cerr << "-------------------\n";
   #endif

   OutputArchive (std::cerr) << x; std::cerr << "\n";
   OutputArchive (std::cerr) << y; std::cerr << "\n";
   OutputArchive (std::cerr) << z; std::cerr << "\n";
   OutputArchive (std::cerr) << w; std::cerr << "\n";

   X a, b, c, d;

   InputArchive (*new std::istringstream {"{}"}) >> a; dump(a);
   InputArchive (*new std::istringstream {R"({"a":45})"}) >> b; dump(b);
   InputArchive (*new std::istringstream {R"({"b":66})"}) >> c; dump(c);
   InputArchive (*new std::istringstream {R"({"a":45, "b":100})"}) >> d; dump(d);
   InputArchive (*new std::istringstream {R"({"b":45, "a":100})"}) >> d; dump(d);

   std::cerr << "-------------------\n";

   std::optional<int> single = {};
   Property<int> single2 = {};

   OutputArchive (std::cerr) << single; std::cerr << "\n";
   OutputArchive (std::cerr) << single2; std::cerr << "\n";

   single = 7865;
   single2.Set(2345);

   OutputArchive (std::cerr) << single; std::cerr << "\n";
   OutputArchive (std::cerr) << single2; std::cerr << "\n";

   std::cerr << "-------------------\n";

   std::vector<std::optional<int>> vout = {{}, 5, 3};
   std::vector<Property<int>> vout2(4); vout2[0].Set(17); vout2[2].Set(2); vout2[3].Set(88);

   OutputArchive (std::cerr) << vout; std::cerr << "\n";
   OutputArchive (std::cerr) << vout2; std::cerr << "\n";

   std::cerr << "-------------------\n";

   std::vector<std::optional<int>> vin;

   InputArchive (*new std::istringstream {R"([1,[2],[],4,[5]])"}) >> vin;
   for (auto e : vin)
      std::cerr << e.value_or(-432) << "\n";

   std::vector<Property<int>> vin2;

   InputArchive (*new std::istringstream {R"([[1],2,[3],[],5])"}) >> vin2;
   for (auto e : vin2)
      std::cerr << (e.IsSet()? e.Get() : -543) << "\n";

   std::cerr << "-------------------\n";

   Y yy;

   OutputArchive (std::cerr) << yy; std::cerr << "\n";

   yy.x1 = c;

   OutputArchive (std::cerr) << yy; std::cerr << "\n";

   yy.x2.Set(d);

   OutputArchive (std::cerr) << yy; std::cerr << "\n";

   yy.x1 = d; yy.x2.Set(c);

   OutputArchive (std::cerr) << yy; std::cerr << "\n";

   OutputArchive (std::cerr) << yy.x2; std::cerr << "\n";
   yy.x2 = {};
   OutputArchive (std::cerr) << yy.x2; std::cerr << "\n";

   std::cerr << "-------------------\n";

   std::optional<Y> yyopt;
   OutputArchive (std::cerr) << yyopt; std::cerr << "\n";
   yyopt = yy;
   OutputArchive (std::cerr) << yyopt; std::cerr << "\n";

   std::cerr << "-------------------\n";

   Y ya, yb, yc, yd;

   InputArchive (*new std::istringstream {R"({"mandatory":{"a":3}}                                                          )"}) >> ya; dump(ya);
   InputArchive (*new std::istringstream {R"({"x1":{"b":66},"mandatory":{"a":3}}                                            )"}) >> yb; dump(yb);
   InputArchive (*new std::istringstream {R"({"x1":{"b":66},"x2":{"a":100,"b":45},"mandatory":{"a":3},"x2":{"a":100,"b":45}})"}) >> yc; dump(yc);
   InputArchive (*new std::istringstream {R"({"x1":{"a":100,"b":45},"x2":{"b":66},"mandatory":{"a":3},"x2":{"b":88}}        )"}) >> yd; dump(yd);
   InputArchive (*new std::istringstream {R"({"x1":{"b":100,"a":45},"x2":{"b":66},"mandatory":{"a":9},"x2":{"b":88}}        )"}) >> yd; dump(yd);
   InputArchive (*new std::istringstream {R"({"x1":{"b":100,"a":45},"mandatory":{"a":9}}                                    )"}) >> yd; dump(yd);
}
