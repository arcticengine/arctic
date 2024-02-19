// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

// The MIT License (MIT)
//
// Copyright (c) 2023 Huldra
// Copyright (c) 2016 Maxim Gumin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "engine/arctic_types.h"
#include "engine/arctic_platform_fatal.h"
#include "engine/log.h"

namespace arctic {

/*
class Helper {
  public static T Get<T>(this XElement xelem, string attribute, T defaultT = default) {
    XAttribute a = xelem.Attribute(attribute);
    return a == null ? defaultT : (T)TypeDescriptor.GetConverter(typeof(T)).ConvertFromInvariantString(a.Value);
  }

  public static IEnumerable<XElement> Elements(this XElement xelement, params string[] names) => xelement.Elements().Where(e => names.Any(n => n == e.Name));
};

class BitmapHelper {
    unsafe public static void SaveBitmap(int[] data, int width, int height, string filename) {
        fixed (int* pData = data) {
            using var image = Image.WrapMemory<Bgra32>(pData, width, height);
            image.SaveAsPng(filename);
        }
    }
};

class Model {
  bool wave[][];
  int propagator[][][];
  int observed[];
  int MX;
  int MY;
  int T;
  int N;
  bool periodic;
  bool ground;
  double weights[];
  int sumsOfOnes[];
  double sumsOfWeights[];
  double sumsOfWeightLogWeights[];
  double entropies[];
  static int dx[] = { -1, 0, 1, 0 };
  static int dy[] = { 0, 1, 0, -1 };
public:
  int compatible[][][];
  (int, int)[] stack;
  int stacksize;
  int observedSoFar;
  double weightLogWeights[];
  double distribution[];
  double sumOfWeights;
  double sumOfWeightLogWeights;
  double startingEntropy;

  enum Heuristic {
    Entropy,
    MRV,
    Scanline
  };
  Heuristic heuristic;
  static int opposite[] = { 2, 3, 0, 1 };
protected:
  Model(int width, int height, int N, bool periodic, Heuristic heuristic) {
    MX = width;
    MY = height;
    this.N = N;
    this.periodic = periodic;
    this.heuristic = heuristic;
  }
public:
  void Init() {
    wave = new bool[MX * MY][];
    compatible = new int[wave.Length][][];
    for (int i = 0; i < wave.Length; i++) {
      wave[i] = new bool[T];
      compatible[i] = new int[T][];
      for (int t = 0; t < T; t++) {
        compatible[i][t] = new int[4];
      }
    }
    distribution = new double[T];
    observed = new int[MX * MY];

    weightLogWeights = new double[T];
    sumOfWeights = 0;
    sumOfWeightLogWeights = 0;

    for (int t = 0; t < T; t++) {
      weightLogWeights[t] = weights[t] * Math.Log(weights[t]);
      sumOfWeights += weights[t];
      sumOfWeightLogWeights += weightLogWeights[t];
    }

    startingEntropy = Math.Log(sumOfWeights) - sumOfWeightLogWeights / sumOfWeights;

    sumsOfOnes = new int[MX * MY];
    sumsOfWeights = new double[MX * MY];
    sumsOfWeightLogWeights = new double[MX * MY];
    entropies = new double[MX * MY];

    stack = new (int, int)[wave.Length * T];
    stacksize = 0;
  }

  bool Run(int seed, int limit) {
    if (wave == null) {
      Init();
    }
    Clear();
    Random random = new(seed);

    for (int l = 0; l < limit || limit < 0; l++) {
      int node = NextUnobservedNode(random);
      if (node >= 0) {
        Observe(node, random);
        bool success = Propagate();
        if (!success) {
          return false;
        }
      } else {
        for (int i = 0; i < wave.Length; i++) {
          for (int t = 0; t < T; t++) {
            if (wave[i][t]) {
              observed[i] = t;
              break;
            }
          }
        }
        return true;
      }
    }
    return true;
  }

  int NextUnobservedNode(Random random) {
    if (heuristic == Heuristic.Scanline) {
      for (int i = observedSoFar; i < wave.Length; i++) {
        if (!periodic && (i % MX + N > MX || i / MX + N > MY)) {
          continue;
        }
        if (sumsOfOnes[i] > 1) {
          observedSoFar = i + 1;
          return i;
        }
      }
      return -1;
    }

    double min = 1E+4;
    int argmin = -1;
    for (int i = 0; i < wave.Length; i++) {
      if (!periodic && (i % MX + N > MX || i / MX + N > MY)) {
        continue;
      }
      int remainingValues = sumsOfOnes[i];
      double entropy = heuristic == Heuristic.Entropy ? entropies[i] : remainingValues;
      if (remainingValues > 1 && entropy <= min) {
        double noise = 1E-6 * random.NextDouble();
        if (entropy + noise < min) {
          min = entropy + noise;
          argmin = i;
        }
      }
    }
    return argmin;
  }

  void Observe(int node, Random random) {
    bool[] w = wave[node];
    for (int t = 0; t < T; t++) {
      distribution[t] = w[t] ? weights[t] : 0.0;
    }
    int r = distribution.Random(random.NextDouble());
    for (int t = 0; t < T; t++) {
      if (w[t] != (t == r)) {
        Ban(node, t);
      }
    }
  }

  bool Propagate() {
    while (stacksize > 0) {
      (int i1, int t1) = stack[stacksize - 1];
      stacksize--;

      int x1 = i1 % MX;
      int y1 = i1 / MX;

      for (int d = 0; d < 4; d++) {
        int x2 = x1 + dx[d];
        int y2 = y1 + dy[d];
        if (!periodic && (x2 < 0 || y2 < 0 || x2 + N > MX || y2 + N > MY)) {
          continue;
        }

        if (x2 < 0) {
          x2 += MX;
        } else if (x2 >= MX) {
          x2 -= MX;
        } if (y2 < 0) {
          y2 += MY;
        } else if (y2 >= MY) {
          y2 -= MY;
        }

        int i2 = x2 + y2 * MX;
        int[] p = propagator[d][t1];
        int[][] compat = compatible[i2];

        for (int l = 0; l < p.Length; l++) {
          int t2 = p[l];
          int[] comp = compat[t2];

          comp[d]--;
          if (comp[d] == 0) {
            Ban(i2, t2);
          }
        }
      }
    }

    return sumsOfOnes[0] > 0;
  }

  void Ban(int i, int t) {
    wave[i][t] = false;

    int[] comp = compatible[i][t];
    for (int d = 0; d < 4; d++) {
      comp[d] = 0;
    }
    stack[stacksize] = (i, t);
    stacksize++;

    sumsOfOnes[i] -= 1;
    sumsOfWeights[i] -= weights[t];
    sumsOfWeightLogWeights[i] -= weightLogWeights[t];

    double sum = sumsOfWeights[i];
    entropies[i] = Math.Log(sum) - sumsOfWeightLogWeights[i] / sum;
  }

  void Clear() {
    for (int i = 0; i < wave.Length; i++) {
      for (int t = 0; t < T; t++) {
        wave[i][t] = true;
        for (int d = 0; d < 4; d++) {
          compatible[i][t][d] = propagator[opposite[d]][t].Length;
        }
      }

      sumsOfOnes[i] = weights.Length;
      sumsOfWeights[i] = sumOfWeights;
      sumsOfWeightLogWeights[i] = sumOfWeightLogWeights;
      entropies[i] = startingEntropy;
      observed[i] = -1;
    }
    observedSoFar = 0;

    if (ground) {
      for (int x = 0; x < MX; x++) {
        for (int t = 0; t < T - 1; t++) {
          Ban(x + (MY - 1) * MX, t);
        }
        for (int y = 0; y < MY - 1; y++) {
          Ban(x + y * MX, T - 1);
        }
      }
      Propagate();
    }
  }

  vitrual void Save(string filename) = 0;
}

class OverlappingModel : Model {
  List<byte[]> patterns;
  List<int> colors;

  public OverlappingModel(string name, int N, int width, int height, bool periodicInput, bool periodic, int symmetry, bool ground, Heuristic heuristic)
  : base(width, height, N, periodic, heuristic) {
    Sprite bitmap;
    bitmap.Load("samples/" + name + ".png");
    int SX = bitmap.GetWidth();
    int SY = bitmap.GetHeight();

    byte[] sample = new byte[bitmap.Length];
    colors = new List<int>();
    for (int i = 0; i < sample.Length; i++) {
      int color = bitmap[i];
      int k = 0;
      for (; k < colors.Count; k++) {
        if (colors[k] == color) {
          break;
        }
      }
      if (k == colors.Count) {
        colors.Add(color);
      }
      sample[i] = (byte)k;
    }

    static byte[] pattern(Func<int, int, byte> f, int N) {
      byte[] result = new byte[N * N];
      for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
          result[x + y * N] = f(x, y);
        }
      }
      return result;
    };
    static byte[] rotate(byte[] p, int N) => pattern((x, y) => p[N - 1 - y + x * N], N);
    static byte[] reflect(byte[] p, int N) => pattern((x, y) => p[N - 1 - x + y * N], N);

    static long hash(byte[] p, int C) {
      long result = 0, power = 1;
      for (int i = 0; i < p.Length; i++) {
        result += p[p.Length - 1 - i] * power;
        power *= C;
      }
      return result;
    };

    patterns = new();
    Dictionary<long, int> patternIndices = new();
    List<double> weightList = new();

    int C = colors.Count;
    int xmax = periodicInput ? SX : SX - N + 1;
    int ymax = periodicInput ? SY : SY - N + 1;
    for (int y = 0; y < ymax; y++) {
      for (int x = 0; x < xmax; x++) {
        byte[][] ps = new byte[8][];

        ps[0] = pattern((dx, dy) => sample[(x + dx) % SX + (y + dy) % SY * SX], N);
        ps[1] = reflect(ps[0], N);
        ps[2] = rotate(ps[0], N);
        ps[3] = reflect(ps[2], N);
        ps[4] = rotate(ps[2], N);
        ps[5] = reflect(ps[4], N);
        ps[6] = rotate(ps[4], N);
        ps[7] = reflect(ps[6], N);

        for (int k = 0; k < symmetry; k++) {
          byte[] p = ps[k];
          long h = hash(p, C);
          if (patternIndices.TryGetValue(h, out int index)) {
            weightList[index] = weightList[index] + 1;
          } else {
            patternIndices.Add(h, weightList.Count);
            weightList.Add(1.0);
            patterns.Add(p);
          }
        }
      }
    }

    weights = weightList.ToArray();
    T = weights.Length;
    this.ground = ground;

    static bool agrees(byte[] p1, byte[] p2, int dx, int dy, int N) {
      int xmin = dx < 0 ? 0 : dx, xmax = dx < 0 ? dx + N : N, ymin = dy < 0 ? 0 : dy, ymax = dy < 0 ? dy + N : N;
      for (int y = ymin; y < ymax; y++) {
        for (int x = xmin; x < xmax; x++) {
          if (p1[x + N * y] != p2[x - dx + N * (y - dy)]) {
            return false;
          }
        }
      }
      return true;
    };

    propagator = new int[4][][];
    for (int d = 0; d < 4; d++) {
      propagator[d] = new int[T][];
      for (int t = 0; t < T; t++) {
        List<int> list = new();
        for (int t2 = 0; t2 < T; t2++) {
          if (agrees(patterns[t], patterns[t2], dx[d], dy[d], N)) {
            list.Add(t2);
          }
        }
        propagator[d][t] = new int[list.Count];
        for (int c = 0; c < list.Count; c++) {
          propagator[d][t][c] = list[c];
        }
      }
    }
  }

  public override void Save(string filename) {
    int[] bitmap = new int[MX * MY];
    if (observed[0] >= 0) {
      for (int y = 0; y < MY; y++) {
        int dy = y < MY - N + 1 ? 0 : N - 1;
        for (int x = 0; x < MX; x++) {
          int dx = x < MX - N + 1 ? 0 : N - 1;
          bitmap[x + y * MX] = colors[patterns[observed[x - dx + (y - dy) * MX]][dx + dy * N]];
        }
      }
    } else {
      for (int i = 0; i < wave.Length; i++) {
        int contributors = 0, r = 0, g = 0, b = 0;
        int x = i % MX, y = i / MX;
        for (int dy = 0; dy < N; dy++) {
          for (int dx = 0; dx < N; dx++) {
            int sx = x - dx;
            if (sx < 0) {
              sx += MX;
            }
            int sy = y - dy;
            if (sy < 0) {
              sy += MY;
            }
            int s = sx + sy * MX;
            if (!periodic && (sx + N > MX || sy + N > MY || sx < 0 || sy < 0)) {
              continue;
            }
            for (int t = 0; t < T; t++) {
              if (wave[s][t]) {
                contributors++;
                int argb = colors[patterns[t][dx + dy * N]];
                r += (argb & 0xff0000) >> 16;
                g += (argb & 0xff00) >> 8;
                b += argb & 0xff;
              }
            }
          }
        }
        bitmap[i] = unchecked((int)0xff000000 | ((r / contributors) << 16) | ((g / contributors) << 8) | b / contributors);
      }
    }
    BitmapHelper.SaveBitmap(bitmap, MX, MY, filename);
  }
}

class SimpleTiledModel : Model {
  List<int[]> tiles;
  List<string> tilenames;
  int tilesize;
  bool blackBackground;

  public SimpleTiledModel(string name, string subsetName, int width, int height, bool periodic, bool blackBackground, Heuristic heuristic) : base(width, height, 1, periodic, heuristic) {
    this.blackBackground = blackBackground;
    XElement xroot = XDocument.Load($"tilesets/{name}.xml").Root;
    bool unique = xroot.Get("unique", false);

    List<string> subset = null;
    if (subsetName != null) {
      XElement xsubset = xroot.Element("subsets").Elements("subset").FirstOrDefault(x => x.Get<string>("name") == subsetName);
      if (xsubset == null) {
        Console.WriteLine($"ERROR: subset {subsetName} is not found");
      } else {
        subset = xsubset.Elements("tile").Select(x => x.Get<string>("name")).ToList();
      }
    }

    static int[] tile(Func<int, int, int> f, int size) {
      int[] result = new int[size * size];
      for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
          result[x + y * size] = f(x, y);
        }
      }
      return result;
    };
    static int[] rotate(int[] array, int size) => tile((x, y) => array[size - 1 - y + x * size], size);
    static int[] reflect(int[] array, int size) => tile((x, y) => array[size - 1 - x + y * size], size);

    tiles = new List<int[]>();
    tilenames = new List<string>();
    var weightList = new List<double>();

    var action = new List<int[]>();
    var firstOccurrence = new Dictionary<string, int>();

    foreach (XElement xtile in xroot.Element("tiles").Elements("tile")) {
      string tilename = xtile.Get<string>("name");
      if (subset != null && !subset.Contains(tilename)) {
        continue;
      }

      Func<int, int> a, b;
      int cardinality;

      char sym = xtile.Get("symmetry", 'X');
      if (sym == 'L') {
        cardinality = 4;
        a = i => (i + 1) % 4;
        b = i => i % 2 == 0 ? i + 1 : i - 1;
      } else if (sym == 'T') {
        cardinality = 4;
        a = i => (i + 1) % 4;
        b = i => i % 2 == 0 ? i : 4 - i;
      } else if (sym == 'I') {
        cardinality = 2;
        a = i => 1 - i;
        b = i => i;
      } else if (sym == '\\') {
        cardinality = 2;
        a = i => 1 - i;
        b = i => 1 - i;
      } else if (sym == 'F') {
        cardinality = 8;
        a = i => i < 4 ? (i + 1) % 4 : 4 + (i - 1) % 4;
        b = i => i < 4 ? i + 4 : i - 4;
      } else {
        cardinality = 1;
        a = i => i;
        b = i => i;
      }

      T = action.Count;
      firstOccurrence.Add(tilename, T);

      int[][] map = new int[cardinality][];
      for (int t = 0; t < cardinality; t++) {
        map[t] = new int[8];

        map[t][0] = t;
        map[t][1] = a(t);
        map[t][2] = a(a(t));
        map[t][3] = a(a(a(t)));
        map[t][4] = b(t);
        map[t][5] = b(a(t));
        map[t][6] = b(a(a(t)));
        map[t][7] = b(a(a(a(t))));

        for (int s = 0; s < 8; s++) {
          map[t][s] += T;
        }

        action.Add(map[t]);
      }

      if (unique) {
        for (int t = 0; t < cardinality; t++) {
          int[] bitmap;
          (bitmap, tilesize, tilesize) = BitmapHelper.LoadBitmap($"tilesets/{name}/{tilename} {t}.png");
          tiles.Add(bitmap);
          tilenames.Add($"{tilename} {t}");
        }
      } else {
        int[] bitmap;
        (bitmap, tilesize, tilesize) = BitmapHelper.LoadBitmap($"tilesets/{name}/{tilename}.png");
        tiles.Add(bitmap);
        tilenames.Add($"{tilename} 0");

        for (int t = 1; t < cardinality; t++) {
          if (t <= 3) tiles.Add(rotate(tiles[T + t - 1], tilesize));
          if (t >= 4) tiles.Add(reflect(tiles[T + t - 4], tilesize));
          tilenames.Add($"{tilename} {t}");
        }
      }

      for (int t = 0; t < cardinality; t++) {
        weightList.Add(xtile.Get("weight", 1.0));
      }
    }

    T = action.Count;
    weights = weightList.ToArray();

    propagator = new int[4][][];
    var densePropagator = new bool[4][][];
    for (int d = 0; d < 4; d++) {
      densePropagator[d] = new bool[T][];
      propagator[d] = new int[T][];
      for (int t = 0; t < T; t++) {
        densePropagator[d][t] = new bool[T];
      }
    }

    foreach (XElement xneighbor in xroot.Element("neighbors").Elements("neighbor")) {
      string[] left = xneighbor.Get<string>("left").Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
      string[] right = xneighbor.Get<string>("right").Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

      if (subset != null && (!subset.Contains(left[0]) || !subset.Contains(right[0]))) {
        continue;
      }

      int L = action[firstOccurrence[left[0]]][left.Length == 1 ? 0 : int.Parse(left[1])], D = action[L][1];
      int R = action[firstOccurrence[right[0]]][right.Length == 1 ? 0 : int.Parse(right[1])], U = action[R][1];

      densePropagator[0][R][L] = true;
      densePropagator[0][action[R][6]][action[L][6]] = true;
      densePropagator[0][action[L][4]][action[R][4]] = true;
      densePropagator[0][action[L][2]][action[R][2]] = true;

      densePropagator[1][U][D] = true;
      densePropagator[1][action[D][6]][action[U][6]] = true;
      densePropagator[1][action[U][4]][action[D][4]] = true;
      densePropagator[1][action[D][2]][action[U][2]] = true;
    }

    for (int t2 = 0; t2 < T; t2++) {
      for (int t1 = 0; t1 < T; t1++) {
        densePropagator[2][t2][t1] = densePropagator[0][t1][t2];
        densePropagator[3][t2][t1] = densePropagator[1][t1][t2];
      }
    }

    List<int>[][] sparsePropagator = new List<int>[4][];
    for (int d = 0; d < 4; d++) {
      sparsePropagator[d] = new List<int>[T];
      for (int t = 0; t < T; t++) {
        sparsePropagator[d][t] = new List<int>();
      }
    }

    for (int d = 0; d < 4; d++) {
      for (int t1 = 0; t1 < T; t1++) {
        List<int> sp = sparsePropagator[d][t1];
        bool[] tp = densePropagator[d][t1];

        for (int t2 = 0; t2 < T; t2++) {
          if (tp[t2]) {
            sp.Add(t2);
          }
        }

        int ST = sp.Count;
        if (ST == 0) {
          Console.WriteLine($"ERROR: tile {tilenames[t1]} has no neighbors in direction {d}");
        }
        propagator[d][t1] = new int[ST];
        for (int st = 0; st < ST; st++) {
          propagator[d][t1][st] = sp[st];
        }
      }
    }
  }

  public override void Save(string filename) {
    int[] bitmapData = new int[MX * MY * tilesize * tilesize];
    if (observed[0] >= 0) {
      for (int x = 0; x < MX; x++) for (int y = 0; y < MY; y++) {
        int[] tile = tiles[observed[x + y * MX]];
        for (int dy = 0; dy < tilesize; dy++) for (int dx = 0; dx < tilesize; dx++) {
          bitmapData[x * tilesize + dx + (y * tilesize + dy) * MX * tilesize] = tile[dx + dy * tilesize];
        }
      }
    } else {
      for (int i = 0; i < wave.Length; i++) {
        int x = i % MX, y = i / MX;
        if (blackBackground && sumsOfOnes[i] == T) {
          for (int yt = 0; yt < tilesize; yt++) {
            for (int xt = 0; xt < tilesize; xt++) {
              bitmapData[x * tilesize + xt + (y * tilesize + yt) * MX * tilesize] = 255 << 24;
            }
          }
        } else {
          bool[] w = wave[i];
          double normalization = 1.0 / sumsOfWeights[i];
          for (int yt = 0; yt < tilesize; yt++) for (int xt = 0; xt < tilesize; xt++) {
            int idi = x * tilesize + xt + (y * tilesize + yt) * MX * tilesize;
            double r = 0, g = 0, b = 0;
            for (int t = 0; t < T; t++) {
              if (w[t]) {
                int argb = tiles[t][xt + yt * tilesize];
                r += ((argb & 0xff0000) >> 16) * weights[t] * normalization;
                g += ((argb & 0xff00) >> 8) * weights[t] * normalization;
                b += (argb & 0xff) * weights[t] * normalization;
              }
            }
            bitmapData[idi] = unchecked((int)0xff000000 | ((int)r << 16) | ((int)g << 8) | (int)b);
          }
        }
      }
    }
    BitmapHelper.SaveBitmap(bitmapData, MX * tilesize, MY * tilesize, filename);
  }

  public string TextOutput() {
    var result = new System.Text.StringBuilder();
    for (int y = 0; y < MY; y++) {
      for (int x = 0; x < MX; x++) {
        result.Append($"{tilenames[observed[x + y * MX]]}, ");
      }
      result.Append(Environment.NewLine);
    }
    return result.ToString();
  }
}

struct Tile {
  enum Symmetry {
    SymmetryI,
    SymmetryX,
    SymmetryL,
    SymmetryT
  };

  Sprite sprite;
  Symmetry symmetry;
};

struct Input {
};

void Main() {
  if (isOverlapping) {
    int N = xelem.Get("N", 3);
    bool periodicInput = xelem.Get("periodicInput", true);
    int symmetry = xelem.Get("symmetry", 8);
    bool ground = xelem.Get("ground", false);
    model = new OverlappingModel(name, N, width, height, periodicInput, periodic, symmetry, ground, heuristic);
  } else {
    string subset = xelem.Get<string>("subset");
    bool blackBackground = xelem.Get("blackBackground", false);
    model = new SimpleTiledModel(name, subset, width, height, periodic, blackBackground, heuristic);
  }

  for (int i = 0; i < xelem.Get("screenshots", 2); i++) {
    for (int k = 0; k < 10; k++) {
      Console.Write("> ");
      int seed = random.Next();
      bool success = model.Run(seed, xelem.Get("limit", -1));
      if (success) {
        Console.WriteLine("DONE");
        model.Save($"output/{name} {seed}.png");
        if (model is SimpleTiledModel stmodel && xelem.Get("textOutput", false)) {
          System.IO.File.WriteAllText($"output/{name} {seed}.txt", stmodel.TextOutput());
        }
        break;
      } else {
        Console.WriteLine("CONTRADICTION");
      }
    }
  }
}
*/

}  // namespace arctic

