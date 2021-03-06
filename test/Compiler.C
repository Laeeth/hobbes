#include <hobbes/hobbes.H>
#include <hobbes/lang/tylift.H>
#include <thread>
#include "test.H"

using namespace hobbes;
static cc& c() { static __thread cc* x = 0; if (!x) { x = new cc(); } return *x; }

typedef std::pair<const char*, size_t> BufferView;

TEST(Compiler, compileToSupportsMoreThanSixArgs) {
  std::string expression =
    "match vegetables spices fruits meat cheese technique occasion skill drink side with \n"
    "| \"carrots\" \"parsley\" \"lemon\" \"ossobuco\" _ \"braising\" \"everyday\" \"basic\" \"wine\" \"polenta\" -> 10 \n"
    "| _ _ _ _ _ _ _ _ _ _ -> -1";

  typedef int (*MatchFunPtr)(const BufferView*,   // vegetables
                             const BufferView*,   // spices
                             const BufferView*,   // fruits
                             const BufferView*,   // meat
                             const BufferView*,   // cheese
                             const BufferView*,   // technique
                             const BufferView*,   // occasion
                             const BufferView*,   // skill
                             const BufferView*,   // drink
                             const BufferView*);  // side
  
  hobbes::cc compiler;

  MatchFunPtr funPtr = hobbes::compileTo<MatchFunPtr>(
    &compiler,
    hobbes::list<std::string>("vegetables", "spices", "fruits",
                              "meat","cheese", "technique",
                              "occasion", "skill", "drink", "side"),
    expression);
  
  EXPECT_TRUE(NULL != funPtr);
}

TEST(Compiler, charArrExpr) {
  char buffer[256];
  strncpy(buffer, "\"hello world\"\0", sizeof(buffer));
  EXPECT_TRUE(c().compileFn<const array<char>*()>(buffer) != 0);
}

class BV { };
namespace hobbes {
  template <>
    struct lift<BV*> : public lift<std::pair<const char*, size_t>*> {
    };
}

TEST(Compiler, liftApathy) {
  // one definition by-ref should be enough to infer the equivalent references
  EXPECT_EQ(show(lift<BV*>::type(c())),       "(<char> * long)");
  EXPECT_EQ(show(lift<const BV*>::type(c())), "(<char> * long)");
  EXPECT_EQ(show(lift<BV&>::type(c())),       "(<char> * long)");
  EXPECT_EQ(show(lift<const BV&>::type(c())), "(<char> * long)");
}

TEST(Compiler, compileFnTypes) {
  EXPECT_EQ(c().compileFn<int(const std::string&)>("_","42")(""), 42);
  EXPECT_EQ(c().compileFn<int(*)(const std::string&)>("_","42")(""), 42);
}

static int appC(const closure<int(int)>& c) { return c(7); }
TEST(Compiler, liftClosTypes) {
  c().bind("appC", &appC);
  EXPECT_EQ(c().compileFn<int()>("(\\x.appC(\\y.x*y-x))(7)")(), 42);
}

TEST(Compiler, Parsing) {
  EXPECT_TRUE((c().compileFn<bool(const std::pair<char,char>&)>("p", "p==('\\\\','\\\\')")(std::make_pair('\\','\\'))));
}

TEST(Compiler, ParseTyDefStaging) {
  compile(
    &c(),
    c().readModule(
      "bob = 42\n"
      "type BT = (TypeOf `bob` x) => x\n"
      "type BTI = (TypeOf `newPrim()::BT` x) => x\n"
      "frank :: BTI\n"
      "frank = 3\n"
    )
  );
  EXPECT_EQ(c().compileFn<int()>("frank")(), 3);
}

TEST(Compiler, ccInManyThreads) {
  std::vector<std::thread*> ps;
  size_t badChecks = 0;
  for (size_t p = 0; p < 10; ++p) {
    ps.push_back(new std::thread(([&]() {
      hobbes::cc c;
      badChecks += c.compileFn<int()>("sum([1..100])-100*101/2")(); // just 0, but complex enough to hit many areas of the compiler
    })));
  }
  for (auto p : ps) { p->join(); delete p; }
  EXPECT_EQ(badChecks, size_t(0));
}

