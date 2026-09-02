// The random generators: seeds, states and their text form.
#define TEST_NO_MAIN
#include "test_helpers.h"

void test_random() {
  {
    float mi = 1.f;
    float ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      float f = GetEngine()->GetRandomF();
      TEST_CHECK_(f >= 0.0f && f < 1.0f, "f=%.16f", f);
      mi = std::min(mi, f);
      ma = std::max(ma, f);
    }
    TEST_CHECK_(ma - mi > 0.9f, "mi=%.16f ma=%.16f", mi, ma);
  }
  {
    double mi = 1.f;
    double ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      double d = GetEngine()->GetRandomD();
      TEST_CHECK_(d >= 0.0 && d < 1.0, "d=%.16f", d);
      mi = std::min(mi, d);
      ma = std::max(ma, d);
    }
    TEST_CHECK_(ma - mi > 0.9, "mi=%.16f ma=%.16f", mi, ma);
  }
  {
    float mi = 1.f;
    float ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      float sf = GetEngine()->GetRandomSF();
      TEST_CHECK_(sf >= -1.0f && sf < 1.0f, "sf=%.16f", sf);
      mi = std::min(mi, sf);
      ma = std::max(ma, sf);
    }
    TEST_CHECK_(ma - mi > 1.9f, "mi=%.16f ma=%.16f", mi, ma);
  }
  {
    double mi = 1.f;
    double ma = -1.f;
    for (Si32 i = 0; i < 1000; ++i) {
      double sd = GetEngine()->GetRandomSD();
      TEST_CHECK_(sd >= -1.0 && sd < 1.0, "sd=%.16f", sd);
      mi = std::min(mi, sd);
      ma = std::max(ma, sd);
    }
    TEST_CHECK_(ma - mi > 1.9, "mi=%.16f ma=%.16f", mi, ma);
  }
}

// The same seed has to give the same numbers, and a different one different
// numbers, or a level generator cannot be reproduced.
void test_random_seed_determinism(void) {
  const Ui64 seed = 20250823ull;
  std::vector<Ui64> first;
  SetRandomSeed(seed);
  for (int i = 0; i < 16; ++i) {
    first.push_back(Random64());
    first.push_back((Ui64)Random32());
    first.push_back((Ui64)Random16());
    first.push_back((Ui64)Random8());
    first.push_back((Ui64)Random(0, 1000000));
  }

  std::vector<Ui64> second;
  SetRandomSeed(seed);
  for (int i = 0; i < 16; ++i) {
    second.push_back(Random64());
    second.push_back((Ui64)Random32());
    second.push_back((Ui64)Random16());
    second.push_back((Ui64)Random8());
    second.push_back((Ui64)Random(0, 1000000));
  }
  TEST_CHECK_(first == second,
      "the same seed gave a different sequence the second time");

  SetRandomSeed(seed + 1ull);
  std::vector<Ui64> other;
  for (size_t i = 0; i < first.size(); ++i) {
    other.push_back(Random64());
  }
  TEST_CHECK_(first != other, "a different seed gave the very same sequence");
}

// Numbers drawn after a state is put back have to repeat the ones drawn after it
// was taken, and every width has to be covered: a state that carries only one of
// the four generators looks right until the first Random8.
static std::vector<Ui64> DrawEveryWidth(int count) {
  std::vector<Ui64> drawn;
  for (int i = 0; i < count; ++i) {
    drawn.push_back(Random64());
    drawn.push_back((Ui64)Random32());
    drawn.push_back((Ui64)Random16());
    drawn.push_back((Ui64)Random8());
    drawn.push_back((Ui64)Random(0, 1000000));
  }
  return drawn;
}

void test_random_state_continues_the_sequence(void) {
  SetRandomSeed(777ull);
  DrawEveryWidth(3);

  RandomState state = GetRandomState();
  std::vector<Ui64> first = DrawEveryWidth(8);
  SetRandomState(state);
  std::vector<Ui64> again = DrawEveryWidth(8);
  TEST_CHECK_(first == again,
      "the restored state gave a different sequence");

  // A state is a value, so the copy taken above still holds while the sequence
  // moves on, and it can be put back a second time.
  DrawEveryWidth(5);
  SetRandomState(state);
  std::vector<Ui64> third = DrawEveryWidth(8);
  TEST_CHECK_(first == third,
      "the state stopped working after the sequence moved on");

  // The state has to be a snapshot, not a reset: continuing from it differs from
  // seeding the same seed again.
  SetRandomSeed(777ull);
  std::vector<Ui64> from_seed = DrawEveryWidth(8);
  TEST_CHECK_(first != from_seed,
      "the state behaves as a fresh seed instead of a snapshot");
}

void test_random_state_text_round_trip(void) {
  SetRandomSeed(31415ull);
  DrawEveryWidth(2);

  const std::string text = GetRandomState().ToString();
  std::vector<Ui64> first = DrawEveryWidth(6);

  RandomState restored;
  TEST_CHECK_(restored.FromString(text), "a state written by ToString failed to read");
  SetRandomState(restored);
  std::vector<Ui64> again = DrawEveryWidth(6);
  TEST_CHECK_(first == again,
      "the state read from text gave a different sequence");

  // Garbage is refused and the state it was read into is left as it was.
  RandomState keeper = GetRandomState();
  const std::string kept = keeper.ToString();
  TEST_CHECK_(!keeper.FromString("not a state at all"),
      "FromString accepted a text that is not a state");
  TEST_CHECK_(keeper.ToString() == kept,
      "a refused FromString changed the state anyway");

  // A truncated text is refused as well: three generators out of four are not a
  // state, and taking them would move the sequence somewhere unpredictable.
  const size_t half = text.size() / 2;
  TEST_CHECK_(!keeper.FromString(text.substr(0, half)),
      "FromString accepted a truncated state");
  TEST_CHECK_(keeper.ToString() == kept,
      "a refused truncated FromString changed the state anyway");
}
