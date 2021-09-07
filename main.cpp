/* Command-line interface
   Copyright (C) 2021 scrubbbbs
   Contact: screubbbebs@gemeaile.com =~ s/e//g
   Project: https://github.com/scrubbbbs/cbird

   This file is part of cbird.

   cbird is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   cbird is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with cbird; if not, see
   <https://www.gnu.org/licenses/>.  */
#include "database.h"
#include "engine.h"
#include "env.h"
#include "hamm.h"
#include "ioutil.h"
#include "media.h"
#include "scanner.h"
#include "qtutil.h"
#include "cimgops.h"
#include "cpu.h"

#include "gui/mediabrowser.h"
#include "gui/mediagrouplistwidget.h"
#include "gui/mediagrouptablewidget.h"
#include "gui/videocomparewidget.h"

#include <algorithm>  // std::max

// globals for lazy init
QString& indexPath() {
  static auto* s = new QString;
  return *s;
}

Engine& engine() {
  static auto* e = new Engine(indexPath(), IndexParams());
  return *e;
}

// for checking the build, don't want to release version
// with avx enabled (probably)
static QStringList buildFlags() {
  QStringList flags;
#ifdef __GNUC__
  flags += "gcc v" __VERSION__;
#endif
#ifdef DEBUG
  flags += "debug";
#endif
#ifdef __x86_64__
  flags += "x86_64";
#endif
#ifdef __SSE2__
  flags += "sse2";
#endif
#ifdef __SSSE3__
  flags += "ssse3";
#endif
#ifdef __FMA__
  flags += "fma";
#endif
#ifdef __SSE4_1__
  flags += "sse4_1";
#endif
#ifdef __SSE4_2__
  flags += "sse4_2";
#endif
#ifdef __POPCNT__
  flags += "popcnt";
#endif
#ifdef __AVX__
  flags += "avx";
#endif
#ifdef __AVX2__
  flags += "avx2";
#endif

  return flags;
}

#define BR "\n"
#define HR "\n======================================================================="
#define H1 "\n\n"
#define H2 "\n"
#define H3 "\n  "

static void printLicense() {
  // clang-format off
  printf(
      H2 "cbird, the Content Based Image Retrieval Database"
      H2 "Copyright (C) 2021 scrubbbbs (scrubbbbs@gmail.com) " CBIRD_HOMEPAGE
      H2 "Licensed to you under the GNU GPL version 2 http://www.gnu.org/licenses"
      H2
      H2 "cbird is free software; you are free to modify an distribute it."
      H2 "There is NO WARRANTY, to the extent permitted by law."
      H2
      HR
      H2
      H2 "This software uses the Qt5 library under GPLv2 https://www.qt.io"
      H2 "This software uses the OpenCV library under Apache License v2 https://opencv.org"
      H2 "This software uses the FFmpeg library under GPLv2 https://ffmpeg.org"
      H2 "This software uses the quazip library under GPLv2 https://github.com/stachenov/quazip"
      H2 "This software uses the exiv2 library under GPLv2 https://exiv2.org"
      H2 "This software uses the CImg library under CeCILL v2.1 https://cimg.eu"
      H2
      H2 "This software includes code from jpegquality by Neal Krawetz,"
      H2 "Hacker Factor Solutions, Copyright 2005-2007."
      H2
  );
  // clang-format on
}

static int printUsage(int argc, char** argv) {
  (void)argc;
  // clang-format off

    printf(
        BR
        H2 "    „__„                     CBIRD"
        H2 "    {o,o}     Content Based Image Retrieval Database"
        H2 "    |)__)       "       CBIRD_HOMEPAGE
        H2 "    -“–“-        license: GPLv2 (see: -license)"
        H2 ""
        BR "Usage: %s [args...]"
        BR
        H2 "* Arguments are positional, and may be given multiple times"
        H2 "* Operations occur in the order given, can often be chained"
        H2 "* Definitions in <>, see last page for help"
        H2 "* Optional values in []"
        H2 "* Alternatives separated |"

        H1 "Setup"
        HR
        H2 "-use <dir>                       set index location, in <dir>/_index, default current directory"
        H2 "-update                          create/refresh index"
        H2 "-headless                        enable most operations without a window manager (must be first argument)"
        H2 "-about                           system information"
        H2 "-h|-help                         help page"
        H2 "-v|-version                      version"
        H2 "-license|--license               show software license and attribution"

        H1 "Queries"
        HR
        H2 "-dups                            exact duplicates using md5 hash"
        H2 "-dups-in <selector>              exact duplicates in subset"
        H2 "-dup-nuke <dir>                  delete (move to trash) dups under <dir> only"
        H2 "-similar                         similar items in entire index"
        H2 "-similar-in <selector>           similar items within a subset"
        H2 "-similar-to <file>|<selector>    similar items to a file, directory, or subset, within entire index"

        H1 "Selections"
        HR
        H2 "* select commands may be chained to append to the current selection"
        H2 "* operations on current selection may clear the selection (-group-by etc)"
        H2 "-select-none                     clear the selection"
        H2 "-select-all                      everything"
        H2 "-select-id <integer>             one item by its unique id"
        H2 "-select-one <file>               one item by path"
        H2 "-select-path <selector>          subset by path, using selector"
        H2 "-select-type <type>              subset by type"
        H2 "-select-errors                   items with errors from last operation (-update,-verify)"
        H2 "-select-result                   to chain queries: convert the last query result into a selection, and clear the result"
        H2 "-select-sql <sql>                items with sql statement [select * from media where ...]"
        H2 "-select-files <file> [<file>]... ignore index, existing files of supported file types"
        H2 "-select-grid <file>              ignore index, detect a grid of thumbnails, break up into separate images"

        H1 "Filtering"
        HR
        H2 "-with <prop>[:<func>] <comparator>    remove items if comparator is false"
        H2 "-without <prop>[:<func>] <comparator> inversion of -with"
        H2 "-first                                keep only the first item"
        H2 "-chop                                 remove the first item"
        H2 "-first-sibling                        keep one item from each directory"

        H1 "Sorting/Grouping"
        HR
        H2 "-sort <prop>[:<func>]               sort ascending"
        H2 "-sort-rev <prop>[:<func>]           sort descending"
        H2 "-group-by <prop>[:<func>]           group selection by property, store in result (clears selection)"
        H2 "-sort-similar                       sort selection by similarity"
        H2 "-merge <selector> <selector>        merge two selections by similarity, into a new list, assuming first selection is sorted"


        H1 "Operations on Selection/Results"
        HR
        H2 "-remove                            remove from the index (force re-indexing)"
        H2 "-nuke                              remove from index and move files to trash"
        H2 "-rename <find> <replace> [-vxp]    rename selection with find/replace, ignoring/preserving file extension"
        H2 "    v                              * verbose preview, show what didn't match and won't be renamed"
        H2 "    x                              * execute the rename, by default only preview"
        H2 "    p                              * <find> matches the full path instead of the file name"
        H2 "-move <dir>                        move selection to another location in the index directory"
        H2 "-verify                            verify md5 sums"

        H1 "Viewing"
        HR
        H2 "-folders                         enable group view, group results from the same parent directory"
        H2 "-sets                            enable group view, group results with the same pair of directories"
        H2 "-exit-on-select                  \"select\" action exits with selected index as exit code, < 0 if canceled"
        H2 "-max-per-page <int>              maximum items on one page [10]"
        H2 "-show                            show results browser for the current selection/results"


        H1 "Miscellaneous"
        HR
        H2 "-qualityscore img                no-reference quality score"
        H2 "-simtest testfile                run automated matching test"
        H2 "-jpeg-repair-script <file>       script/program to repair truncated jpeg files (-verify) [~/bin/jpegfix.sh]"
        H2 "-compare-videos <file> <file>    open a pair of videos in compare tool"
        H2 "-view-image <file>               open results browser with file"
        H2 "-test-csv <file>                 read csv of src/dst pairs for a similar-to test, store results in match.csv"
        H2 "-test-video-decoder <file>       test video decoding"
        H2 "-test-video <file>               test video search"
        H2 "-vacuum                          compact/optimize database files"


        H1 "Search Parameters (for -similar*)"
        HR
        H2 "-p.<key> value                   set search parameters"
        H2 "-p:<key> value                   (alternate)"
        H2 "*all values are integer (0=disable, 1=enable)"
        H2 "*default value in [ ]"
        H2 "*must appear before -similar or other queries to take effect"
        H2 "*keys:"
        H3 "alg   search algorithm ([0]=dct,1=dctFeatures,2=cvFeatures,3=histogramHash,4=dctVideo)"
        H3 "  *dct            discrete cosine transform hash (scale)"
        H3 "  *dctFeatures    dct hashes of features         (scale, crop)"
        H3 "  *cvFeatures     opencv features                (scale, crop, rotate)"
        H3 "  *colorHistogram histogram of colors            (scale, crop, rotate, mirror)"
        H3 "  *dctVideo       dct hashes of video frames     (scale)"

        H3 "dht   dct hash threshold (0-64), lower=more similar [5]"
        H3 "cvt   cv feature matcher threshold, lower=more similar [25]"
        H3 "mm    max matches to show after sorting [5]"
        H3 "tm    validate matches with high-resolution affine template match [0]"
        H3 "nf    number of needle features for template match [100]"
        H3 "hf    number of haystack features for template match [1000]"
        H3 "stat  enable statistics, 1=timing [0]"
        H3 "neg   remove negative matches [0]"
        H3 "mmask try mirrored images ([0]=none,1=horz,2=vert,4=horz+vert,all=7)"
        H3 "qt    query types (CSV) [1]=image,2=video,3=audio"
        H3 "rt    result types (CSV) 1=image,2=video,3=audio [1,2]"
        H3 "vpad  frames to skip at start/end of videos from search [300]"
        H3 "fg    filter-groups: remove duplicate groups {a,b}=={b,a} (same items with different order) [1]"
        H3 "fp    filter-parent: remove groups with all items in the same parent directory [0]"
        H3 "fs    filter-self: remove needle matching itself [1]"
        H3 "mg    merge-groups: merge n-connected groups (currently only 1 supported) [0]"
        H3 "eg    expand-groups: if a matches {b,c}, make two groups {a,b} and {a,c} [0]"

        H1 "Index Parameters (for -update)"
        HR
        H2 "-i.<key> value                  set index parameters"
        H2 "-i:<key> value                  (alternate)"
        H2 "* all values are integers (0=disable, 1=enable)"
        H2 "* default value in [ ]"
        H2 "* must appear before -update to take effect"
        H3 "rec    enable/disable recursive scan [1]"
        H3 "algos  enabled search algos mask [15]"
        H3 "crop   enable/disable border detection/cropping of video prior to indexing [1]"
        H3 "types  enabled media types mask (1=image, 2=video) [3]"
        H3 "hwdec  enable hardware video decoder [0]"
        H3 "decthr max thread count for one video decoder (0==auto) [0]"
        H3 "idxthr max thread count for index jobs (0==auto) [0]"
        H3 "ljf    video only: estimate job cost and process longest jobs first [1]"
        H3 "dry    enable dry-run, only show what would be updated [0]"
        H3 "bsize  size of database write batches to hide write latency [1024]"

        H1 "Definitions"
        HR
        H2 "<file>                          path to file"
        H2 "<dir>                           path to directory"
        H2 "<item>                          file in the index"
        H2 "<group>                         list of items, usually a needle/target and its matches"
        H2 "<result>                        list of groups, from the last query"
        H2 "<selection>                     current list of items for further operations"
        H2 "<selector>                      defines a set of (indexed) items by path, matching expression"
        H2 "    :<regular-expression>       - pcre, prefixed with colon"
        H2 "    [<dir>]<sql-like>           - SQL LIKE expression (case-sensitive) optionally prefixed with dir"
        H2 "    <dir>                       - everything under this path"
        H2 "    @                           - use the current selection"
        H2 "<type>                          item media type (1=image,2=video,3=audio)"
        H2 "<find>                          source expression for string find/replace"
        H2 "    <regular-expression>        - pcre with optional captures for <replace>"
        H2 "    *                           - shortcut for entire string (^.*$)"
        H2 "<replace>                       destination expression for string find/replace"
        H2 "    <template>                  - replace entire string, must contain at least one capture"
        H2 "    <string>                    - replace whole-words/strings, may not contain any capture"
        H2 "    #0 #1 .. #n                 - capture: the nth capture from <find>, #0 captures the entire string"
        H2 "    %%1                          - special: the sequence number, with automatic zero-padding"
        H2 "    {arg:<func>[:<func>]...}    - special: transform arg (after capture/special expansion) with function(s)"
        H2 "<binop>                         logical operators for expressions (comparator)"
        H2 "    ==                          - equal to"
        H2 "    =                           - equal to"
        H2 "    !=                          - not equal to"
        H2 "    <, <=, >, >=                - less-than/greater-than"
        H2 "    ~                           - contains"
        H2 "    !                           - does not contain"
        H2 "<comparator>                    expression to test a value, returns true or false, default operator =="
        H2 "    :<regular-expression>       - pcre matches any part of value"
        H2 "    [<binop>]<string>           - compare using operator, string is converted to value's type"
        H2 "    ~null                       - tests if value is null/missing vs empty"
        H2 "<prop>                          item property for sorting, grouping, filtering"
        H2 "    id                          - unique id"
        H2 "    isValid                     - 1 if id != 0"
        H2 "    md5                         - checksum"
        H2 "    type                        - 1=image,2=video,3=audio"
        H2 "    path                        - file path"
        H2 "    parentPath                  - parent path (dirpath)"
        H2 "    name                        - file name"
        H2 "    archivePath                 - archive/zip path, or empty"
        H2 "    suffix                      - file suffix"
        H2 "    isArchived                  - 1 if archive member"
        H2 "    archiveCount                - number of archive members"

        H2 "    contentType                 - mime content type"
        H2 "    width                       - pixel width"
        H2 "    height                      - pixel height"
        H2 "    resolution                  - width*height"
        H2 "    res                         - max of width, height"
        H2 "    compressionRatio            - resolution / file size"

        H2 "    score                       - match score"
        H2 "    matchFlags                  - match flags (Media::matchFlags)"

        H2 "    exif:<tag1[,tagN]>          - comma-separated exif tags, first available tag is used (\"Exif.\" prefix optional)"
        H2 "                                  see: https://www.exiv2.org/tags.html"
        H2 "    ffmeta:<tag1[,tagN]         - comma-separated ffmpeg metadata tags, first available tag is used"
        H2 "<func>                          transform a property value or string"
        H2 "    mid,from,len                - substring from index (from) with length (len) (see: QString::mid)"
        H2 "    trim                        - remove whitespace from beginning/end"
        H2 "    upper                       - uppercase"
        H2 "    lower                       - lowercase"
        H2 "    title                       - capitalize first letter"
        H2 "    date,<format-string>        - parse value as date and format as string (see QDateTime.toString())"
        H2 "    year                        - shortcut for date,yyyy"
        H2 "    month                       - shortcut for date,yyyy-MM"
        H2 "    day                         - shortcut for date,yyyy-MM-dd"
        H2 "    split,<regexp|string>       - split into array with regexp or string"
        H2 "    camelsplit                  - split into array on uppercase/lowercase (camelCase => [camel, Case])"
        H2 "    join,<string>               - join array with string"
        H2 "    push,<string>               - append string to end of array"
        H2 "    pop                         - remove string from end of array"
        H2 "    foreach,<func>[|func]...    - apply function(s) separated by pipe (|) to each array element"
        H2 "    add,<integer>               - add integer arg to value"
        H2 "    pad,<integer>               - pad integer with zeros with argument width"

        H1 "Examples"
        HR
        H2 "create index in cwd             cbird -update"
        H2 "find exact duplicates           cbird -update -dups -show"
        H2 "find near duplicates            cbird -update -similar -show"
        H2 "find near duplicates (video)    cbird -update -p.alg 4 -p.dht 7 -p.qt 2 -p.vpad 1000 -similar -show"
        H2 "group photo sets by month       cbird -select-type 1 -group-by exif:Photo.DateTimeOriginal:month -folders -show"
        H2 "browse items, 16 per page       cbird -select-all -max-per-page 16 -show"
        BR
        ,argv[0]
        );

  // clang-format on
  return 0;
}

/// print command line completions (e.g. for bash), pipe (|) separated
int printCompletions(const char* argv0, const QStringList& args) {
  //
  // for bash, we need:
  // function _cbird { OIFS=$IFS; IFS='|'; COMPREPLY=($(cbird -complete
  // $COMP_CWORD ${COMP_WORDS[*]})); IFS=$OIFS; } complete -F _cbird cbird
  //
  // cword is the $CWORD variable in bash... index of the cursor (in words)
  if (args.count() < 3) {
    printf(
        "completions require $CWORD as 2nd argument\nusage:%s -complete $CWORD "
        "[args]\n",
        argv0);
    exit(0);
  }

  QStringList cmds;

  const QStringList noArgs{
      "-update",         "-headless",      "-dups",          "-similar",
      "-select-none",    "-select-all",    "-select-errors", "-first",
      "-chop",           "-first-sibling", "-sort-similar",  "-remove",
      "-nuke",           "-rename",        "-sets",          "-folders",
      "-exit-on-select", "-show",          "-help",          "-version",
      "-about",          "-verify",        "-vacuum",        "-select-result"
      "-license"};
  cmds << noArgs;

  QStringList oneArg{"-select-id", "-select-type", "-select-sql",  "-sort",
                     "-sort-rev",  "-group-by",    "-max-per-page"};

  const QStringList twoArg{"-with", "-without", "-merge", "-rename",
                           "-compare-videos"};
  cmds << twoArg;

  const QStringList fileArg{"-select-one",         "-jpeg-repair-script",
                            "-view-image",         "-test-csv",
                            "-test-video-decoder", "-select-grid"};
  cmds << fileArg;

  const QStringList dirArg{"-use",        "-dups-in", "-dup-nuke",
                           "-similar-in", "-move"};
  cmds << dirArg;

  const QStringList fileOrDirArg{"-similar-to", "-select-path"};
  cmds << fileOrDirArg;

  const QStringList paramKeys = {"alg",  "dht",  "cvt", "mm",    "tm", "nf",
                                 "hf",   "stat", "neg", "mmask", "qt", "rt",
                                 "vpad", "fg",   "fp",  "fs",    "mg", "eg"};
  for (auto& k : paramKeys) oneArg << ("-p." + k);

  const QStringList indexKeys = {"rec",    "algos",  "crop", "types", "hwdec",
                                 "decthr", "idxthr", "ljf",  "dry", "bsize"};
  for (auto& k : indexKeys) oneArg << ("-i." + k);

  cmds << oneArg;

  const QStringList argTypeFile{"-compare-videos"};

  cmds.sort();

  int cword = args.at(2).toInt() + 3;

  // completions write to stdout so we cannot log there
#ifdef DEBUG_COMPLETIONS
  QFile log(QDir::tempPath() + "/cbird-completions.log");
  Q_ASSERT(log.open(QFile::WriteOnly | QFile::Append));
#else
  QString log;
#endif

  QDebug debug(&log);
  debug << args << "\n";

  QStringList completions;
  const QString curr = cword >= args.count() ? "" : args[cword];
  const QString prev = cword > 4 && args.count() > 4 ? args[cword - 1] : "";
  const QString prev1 = cword > 5 && args.count() > 5 ? args[cword - 2] : "";

  debug << "curr:" << curr << "prev:" << prev << "prev1:" << prev1 << "\n";

  auto completePath = [&](const QDir::Filters& filter) {
    // fixme: does not work if path contains a space
    QString path = curr;
    path.replace("\\", "");  // remove shell escape so QFileInfo works
    path = path.trimmed();

    bool quote = false; // remove leading quote, in which case we do not escape output
    if (path.startsWith("\"") || path.startsWith("\'")) {
      quote = true;
      path = path.mid(1);
    }

    const QString homePath = QDir::homePath();
    const bool isHome = path.startsWith("~");
    if (isHome) path = homePath + path.mid(1);  // QFileInfo doesn't recognize ~

    const QFileInfo info(path);
    const QDir dir = info.dir();  // dir containing path

    // prefix for dir entries must be compatible with query(curr),
    // use (prefix+entry).startsWith(curr) for the match
    QString prefix = dir.absolutePath();
    if (info.isRelative()) prefix = QDir().relativeFilePath(prefix);
    if (isHome)
      prefix = "~" + prefix.mid(homePath.length());  // keep ~ when matching

    prefix += "/";

    // the prefix will only start with ./ if listing cwd,
    // we only want it if curr also starts with ./
    if (prefix.startsWith("./")) prefix = prefix.mid(2);
    if (curr.startsWith("./")) prefix = "./" + prefix;

    debug << "isDir" << info.isDir() << path << dir.exists() << "\n";
    debug << "isFile" << info.isFile() << "\n";

    bool accept = false;
    if (!accept) accept = (filter & QDir::Files) && info.isFile();
    if (!accept) accept = (filter & QDir::Dirs) && info.isDir();

    // if curr ends with /, list the dir contents, even if
    // we are looing for a dir
    if (accept && path != "" && !path.endsWith("/"))
      // fixme: this is only valid if also no alternatives
      debug << "path exists, no completions";  // acceptable value, no completions
    else {
      const auto entries =
          dir.entryInfoList(filter | QDir::Dirs | QDir::NoDotAndDotDot);
      if ((path == "" || info.exists()) && entries.count() > 1)
        for (auto& e : entries)
          completions << e.fileName();  // names only, if possible
      else
        for (auto& e : entries) {  // match curr to name
          QString comp = prefix + e.fileName();
          if (!quote) comp = comp.replace(" ", "\\ ");
          if (!(filter & QDir::Dirs) && e.isDir()) comp += "/";
          debug << "check-prefix" << path << comp << "\n";
          if (comp.startsWith(path)) completions << comp;
        }
    }
  };

  if (fileOrDirArg.contains(prev)) {
    completePath(QDir::Files | QDir::Dirs);
  } else if (dirArg.contains(prev)) {
    completePath(QDir::Dirs);
  } else if (fileArg.contains(prev)) {
    completePath(QDir::Files);
  } else if (oneArg.contains(prev) || twoArg.contains(prev)) {
    if (argTypeFile.contains(prev)) completePath(QDir::Files);
  } else if (twoArg.contains(prev1)) {
    if (argTypeFile.contains(prev1)) completePath(QDir::Files);
  } else if (curr.startsWith("-") && !cmds.contains(curr)) {
    for (auto& cmd : cmds)
      if (cmd.startsWith(curr)) completions << cmd;
  }

  for (auto& c : completions) debug << "output:" << c << "\n";

  printf("%s\n", qPrintable(completions.join("|")));

  return 0;
}

/// Support <comparator> argument type
class Comparator {
 private:
  std::function<QVariant(const QString&)> _convert;
  std::function<bool(const QVariant&)> _operator;
  QString _valueExp;
  QVariant _value;
  QRegularExpression _re;

  Comparator() = delete;

  void parseOperator(const QString& str) {
    if (str.startsWith("==")) {
      _value = _convert(str.mid(2));
      _operator = [&](const QVariant& v) { return v == _value; };
    } else if (str.startsWith("!=")) {
      _value = _convert(str.mid(2));
      _operator = [&](const QVariant& v) { return v != _value; };
    } else if (str.startsWith("<=")) {
      _value = _convert(str.mid(2));
      _operator = [&](const QVariant& v) { return v <= _value; };
    } else if (str.startsWith(">=")) {
      _value = _convert(str.mid(2));
      _operator = [&](const QVariant& v) { return v >= _value; };
    } else if (str.startsWith("=")) {
      _value = _convert(str.mid(1));
      _operator = [&](const QVariant& v) { return v == _value; };
    } else if (str.startsWith("<")) {
      _value = _convert(str.mid(1));
      _operator = [&](const QVariant& v) { return v < _value; };
    } else if (str.startsWith(">")) {
      _value = _convert(str.mid(1));
      _operator = [&](const QVariant& v) { return v > _value; };
    } else if (str.startsWith("~")) {
      _value = _convert(str.mid(1));
      _operator = [&](const QVariant& v) { return v.toString().contains(_value.toString()); };
    } else if (str.startsWith("!")) {
      _value = _convert(str.mid(1));
      _operator = [&](const QVariant& v) { return !v.toString().contains(_value.toString()); };
    } else {
      _value = _convert(str);
      _operator = [&](const QVariant& v) { return v == _value; };
    }
  }

 public:
  Comparator(const QString& valueExp) {
    QString exp;
    if (valueExp == "~null") {
      _convert = [](const QString& str) { return QVariant(str); };
      _operator = [&](const QVariant& v) { return v.isNull(); };
      _value = QVariant();
    } else if (valueExp.startsWith(":")) {
      _convert = [](const QString& str) { return QVariant(str); };
      _re.setPattern(valueExp.mid(1));
      if (!_re.isValid())
        qFatal("invalid regular expression: %s at offset %d",
               qPrintable(_re.errorString()), _re.patternErrorOffset());
      _operator = [&](const QVariant& v) {
        return _re.match(v.toString()).hasMatch();
      };
    } else {
      _convert = [](const QString& str) { return QVariant(str); };
      parseOperator(valueExp);
    }
  }

  bool compareTo(const QVariant& value) const { return _operator(value); }
};

int main(int argc, char** argv) {
  // parse args ourselves, so we can choose which
  // QApplication type; gui type needed for -show
  QStringList args;
  for (int i = 0; i < argc; i++) args.append(argv[i]);

  if (args.contains("-complete")) return printCompletions(argv[0], args);

  if (args.contains("-h") || args.contains("-help") || args.contains("--help"))
    return printUsage(argc, argv);

  (void)qInstallMessageHandler(qColorMessageOutput);

  args.pop_front();  // discard program name
  QScopedPointer<QCoreApplication> app;
  if (args.contains("-headless"))
    app.reset(new QCoreApplication(argc, argv));
  else
    app.reset(new QApplication(argc, argv));

  app->setApplicationName(CBIRD_PROGNAME);
  app->setApplicationVersion(CBIRD_VERSION);

  if (args.count() <= 0) {
    printUsage(argc, argv);
    return 1;
  }

  // fixme: could this be done lazily?
  VideoContext::loadLibrary();

  SearchParams params;
  IndexParams indexParams;
  MediaGroup selection;        // selection of items by properties
  MediaGroupList queryResult;  // results of a search query

  // set defaults
  indexPath() = ".";
  QString jpegFixPath = "~/bin/jpegfix.sh";

  // show/display options
  int showMode = MediaBrowser::ShowNormal;
  int selectMode = MediaBrowser::SelectSearch;
  int maxPerPage = 10;  // max images on a page (when paging)

  auto sqlEscapePath = [](const QString& path) {
    return QString(path).replace("%", "\\%").replace("_", "\\_");
  };

  // <selector>
  auto selectPath = [&](const QString& pathSpec) {
    QString path = pathSpec;
    if (path == "@") return selection;

    if (path.startsWith(":")) {
      path = path.mid(1);

      const QRegularExpression re(path);
      if (!re.isValid())
        qFatal("invalid regular expression: %s at offset %d",
               qPrintable(re.errorString()), re.patternErrorOffset());

      auto selection = engine().db->mediaWithPathRegexp(path);

      qInfo() << "select all with path regexp ~=" << path << ":"
              << selection.count() << "items";
      return selection;
    } else {
      QFileInfo info(path);
      if (info.exists())
        path = sqlEscapePath(info.absoluteFilePath()) + "%";
      else if (path.contains("/")) {
        QStringList parts = path.split("/");

        // last component should be ignored since info.exists() failed
        QStringList tail;
        tail.append(parts.back());
        parts.pop_back();

        while (parts.count() > 0) {
          // check if we can form a valid path, remove user-escapes
          // (temporarily)
          QString cand = sqlEscapePath(parts.join("/"));

          QFileInfo info(cand);
          if (info.exists()) {
            path = sqlEscapePath(info.absoluteFilePath());
            path += "/" + tail.join("/");
            break;
          }
          // last component didn't yield a valid path, it probably contains
          // wildcards
          tail.prepend(parts.back());
          parts.pop_back();
        }
      }

      // make relative to index, required since database stores relative path
      const QString cwd = sqlEscapePath(QDir::currentPath());
      if (path.startsWith(cwd))
        path = path.mid(engine().db->path().length() + 1);

      if (!path.contains("%")) path.append("%");

      auto selection = engine().db->mediaWithPathLike(path);

      qInfo() << "select all with path like" << path << ":" << selection.count()
              << "items";

      return selection;
    }
    Q_UNREACHABLE();
  };

  //    auto indexRelativePath = [&](const QString& path) {
  //        QString rel = path;
  //        QFileInfo info(rel);
  //        if (info.exists())
  //            rel =
  //            info.absoluteFilePath().mid(engine().db->path().length()+1);
  //        return rel;
  //    };

  auto absolutePath = [](const QString& path) {
    QString abs = path;
    QFileInfo info(abs);
    if (info.exists()) abs = info.absoluteFilePath();
    return abs;
  };

  auto parseType = [](const QString& value) { return value.toInt(); };

  // "arg" always refers to current comand line switch ("-foo")
  QString arg;
  auto nextArg = [&]() {
    if (args.count() > 0) return args.takeFirst();
    qCritical() << "missing argument to" << arg;
    ::exit(1);
  };

  auto intArg = [&](const QString& str) {
    bool ok;
    int val = str.toInt(&ok);
    if (ok) return val;
    qCritical() << arg << "requires an integer value";
    ::exit(1);
  };

  while (args.count() > 0) {
    arg = args.takeFirst();

    // clang-format off
    if (arg.startsWith("-p.") || arg.startsWith("-p:")) {
      const QString val = nextArg();
      const QChar sep = arg[2];
      const QString key = arg.split(sep)[1];
      int intVal = val.contains(",") ? 0 : intArg(val);

      if      (key == "dht") params.dctThresh = intVal;
      else if (key == "cvt") params.cvThresh = intVal;
      else if (key == "alg") params.algo = intVal;
      else if (key == "mn") params.minMatches = intVal;
      else if (key == "mm")  params.maxMatches = intVal;
      else if (key == "nf") params.needleFeatures = intVal;
      else if (key == "hf") params.haystackFeatures = intVal;
      else if (key == "tm") params.templateMatch = intVal;
      else if (key == "stat") params.verbose = intVal;
      else if (key == "neg") params.negativeMatch = intVal;
      else if (key == "mmask") params.mirrorMask = intVal;
      else if (key == "vpad") params.skipFramesIn = params.skipFramesOut = intVal;
      else if (key == "fg") params.filterGroups = intVal;
      else if (key == "mg") params.mergeGroups = intVal;
      else if (key == "fp") params.filterParent = intVal;
      else if (key == "fs") params.filterSelf = intVal;
      else if (key == "eg") params.expandGroups = intVal;
      else if (key == "qt" || key=="rt")  {

          QVector<int> types;
          for (QString tok : val.split(",")) types << QString(tok).toInt();

          if (key == "qt")
            params.queryTypes = types;
          else
            params.resultTypes = types;
      }
      else {
        qCritical() << "invalid search parameter:" << key;
        ::exit(1);
      }
    }
    else if (arg.startsWith("-i.") || arg.startsWith("-i:")) {
      const int intVal = intArg(nextArg());
      const QChar sep = arg[2];
      const QString key = arg.split(sep)[1];

      // todo: implement all index params
      if      (key == "rec")   indexParams.recursive = intVal;
      else if (key == "algos") indexParams.algos = intVal;
      else if (key == "crop")  indexParams.autocrop = intVal;
      else if (key == "types") indexParams.types = intVal;
      else if (key == "hwdec") indexParams.useHardwareDec = intVal;
      else if (key == "decthr") indexParams.decoderThreads = intVal;
      else if (key == "idxthr") indexParams.indexThreads = intVal;
      else if (key == "ljf")    indexParams.estimateCost = intVal;
      else if (key == "bsize")  indexParams.writeBatchSize = intVal;
      else if (key == "dry")    indexParams.dryRun = intVal;
      else {
          qCritical() << "invalid index parameter:" << key;
          ::exit(1);
      }
      engine().scanner->setIndexParams(indexParams);
    }
    // clang-format on
    else if (arg == "-use") {
      const QString path = nextArg();
      if (!QFileInfo(path).isDir())
        qFatal("-use: \"%s\" is not a directory", qPrintable(path));

      indexPath() = path;
    } else if (arg == "-headless") {
      qInfo("selected headless mode");
    } else if (arg == "-update") {
      int threads = indexParams.indexThreads;
      if (threads <= 0) threads = QThread::idealThreadCount();

      QThreadPool::globalInstance()->setMaxThreadCount(threads);

      engine().update(true);

      QThreadPool::globalInstance()->setMaxThreadCount(
          QThread::idealThreadCount());

    } else if (arg == "-about") {
      Scanner* sc = engine().scanner;
      Database* db = engine().db;
      const QStringList ff = VideoContext::ffVersions();
      const QStringList cv = cvVersion();
      const QStringList ev = Media::exifVersion();
      //            const QStringList qv = {"??", "??"};
      qInfo() << CBIRD_PROGNAME << CBIRD_VERSION
              << "[" << CBIRD_GITVERSION << "]" << CBIRD_HOMEPAGE;
      qInfo() << "build:" << buildFlags();
      qInfo() << "Qt" << qVersion() << "compiled:" << QT_VERSION_STR;
      qInfo() << "FFmpeg" << ff[0] << "compiled:" << ff[1];
      qInfo() << "OpenCV" << cv[0] << "compiled:" << cv[1];
      qInfo() << "Exiv2" << ev[0] << "compiled:" << ev[1];
      //            qInfo() << "Quazip" << qv[0] << "compiled:" << qv[1];

      qInfo() << "threads:" << QThread::idealThreadCount();
      qInfo() << "image extensions:" << sc->imageTypes();
      qInfo() << "video extensions:" << sc->videoTypes();
      qInfo() << db->count() << "indexed files";
      qInfo() << db->countType(Media::TypeImage) << "image files";
      qInfo() << db->countType(Media::TypeVideo) << "video files";
      qInfo() << db->countType(Media::TypeAudio) << "audio files";
      qInfo() << "see -license for software license";

    } else if (arg == "-v" || arg == "-version" || arg == "--version") {
      qInfo() << app->applicationName() << app->applicationVersion();
    } else if (arg == "-license" || arg == "--license") {
      printLicense();
    } else if (arg == "-remove") {
      qInfo() << "removing: " << selection.count() << "items";
      //Media::printGroup(selection);
      engine().db->remove(selection);
    } else if (arg == "-dups") {
      queryResult = engine().db->dupsByMd5(params);
      qInfo("dups: %d groups found", queryResult.count());
    } else if (arg == "-dups-in") {
      params.set = selectPath(nextArg());
      params.inSet = true;
      queryResult = engine().db->dupsByMd5(params);
      qInfo("dups-in: %d groups found", queryResult.count());
    } else if (arg == "-dup-nuke") {
      QString path = nextArg();

      QDir dir(path);
      if (!dir.exists()) qFatal("dup-nuke: specified dir does not exist");

      path = dir.absolutePath();

      const MediaGroupList list = engine().db->dupsByMd5(params);

      MediaGroupList filtered;
      for (const MediaGroup& g : list)
        for (const Media& m : g)
          if (m.path().startsWith(path)) filtered.append(g);

      qInfo() << "dup-nuke:" << filtered.count() << "duplicate groups";
      if (filtered.count() <= 0) continue;

      qInfo() << "dup-nuke: removing (at most) one item from each group";

      // fixme: deletion confirmation

      int nuked = 0;
      MediaGroup toRemove;
      for (const MediaGroup& group : filtered) {
        // only remove one of the dups; it could be dup of
        // another in the same path
        // -dups-in <dir> -nuke would delete them all
        for (const Media& m : group)
          if (m.path().startsWith(path)) {
            toRemove.append(m);
            DesktopHelper::moveToTrash(m.path());
            nuked++;
            break;
          }
      }

      qInfo("dup-nuke: %d nuked, updating db", nuked);
      engine().db->remove(toRemove);
    } else if (arg == "-nuke") {

      if (selection.count() > 0) {
        qWarning() << "about to move" << selection.count() << "items to trash.";
        qFlushOutput();
        printf("Proceed? Y/[N]: ");
        fflush(stdout);
        char ch = 'N';
        if (1 == scanf("%c", &ch) && ch == 'Y') {
          engine().db->remove(selection);

          for (auto& m : selection)
            if (!DesktopHelper::moveToTrash(m.path())) exit(-1);

          qInfo() << selection.count() << "nuked";
        }
      } else
        qInfo() << "-nuke: nothing selected";
    } else if (arg == "-similar") {
      queryResult = engine().db->similar(params);
    } else if (arg == "-similar-in") {
      params.set = selectPath(nextArg());
      params.inSet = true;
      queryResult = engine().db->similar(params);
    } else if (arg == "-similar-to") {
      const QString to = nextArg();
      const QFileInfo info(to);

      void* arg = nullptr;
      MediaSearch search;
      search.params = params;

      queryResult.clear();
      MediaGroupList list;

      Scanner* scanner = engine().scanner;

      const QString ext = info.suffix();
      const bool isArchive = scanner->archiveTypes().contains(ext);

      if (info.isFile() && !isArchive) {
        if (params.queryTypes.contains(Media::TypeImage) &&
            scanner->imageTypes().contains(ext)) {
          IndexResult result = scanner->processImageFile(to);
          if (!result.ok) {
            qCritical() << "similar-to: failed to process image file:" << to;
            continue;
          }
          search.needle = result.media;
        } else if (params.queryTypes.contains(Media::TypeVideo) &&
                   scanner->videoTypes().contains(ext)) {
          search.needle = engine().db->mediaWithPath(to);

          // doesn't exist in the database, search by frame grabbing
          if (!search.needle.isValid()) {
            QList<QFuture<MediaSearch>> work;

            IndexParams tmp = indexParams;
            tmp.retainImage = true;
            tmp.autocrop = true;
            tmp.algos = 1 << SearchParams::AlgoDCT;

            scanner->setIndexParams(tmp);

            int numFrames;
            {
              VideoContext v;
              VideoContext::DecodeOptions opt;
              if (0 != v.open(to, opt)) {
                qCritical()
                    << "similar-to: frame grabbing failed to open video:"
                    << arg;
                continue;
              }

              const VideoContext::Metadata md = v.metadata();
              numFrames = int(md.frameRate * md.duration);
            }

            // grab at 10%,20%...90% position
            for (int i = 1; i < 10; i++) {
              int pos = int(numFrames * (i * 0.10));

              work.append(QtConcurrent::run([=] {
                QImage frame = VideoContext::frameGrab(to, pos, true);
                if (!frame.isNull()) {
                  MediaSearch search;
                  search.params = params;

                  search.needle.setType(Media::TypeImage);
                  search.needle.setPath(to);
                  search.needle.setImage(frame);
                  search.needle.setWidth(frame.width());
                  search.needle.setHeight(frame.height());

                  // matches can copy this range as their srcIn value
                  search.needle.setMatchRange(MatchRange(pos, pos, 1));

                  return engine().query(search);
                }
                return MediaSearch();
              }));
            }

            for (auto& w : work) {
              w.waitForFinished();

              MediaSearch result = w.result();

              result.needle.setType(Media::TypeVideo);
              result.needle.setMatchRange(
                  MatchRange());  // technically has no range, messes up viewer

              result.matches.prepend(result.needle);
              if (!engine().db->filterMatch(params, result.matches))
                list.append(result.matches);
              engine().db->filterMatches(params, list);
            }

            scanner->setIndexParams(indexParams);  // restore

            if (list.count() <= 0)
              qInfo("similar-to[external video]: %s: No matches",
                    qPrintable(to));

            queryResult = list;
            continue;
          }
        } else {
          qCritical(
              "similar-to: invalid query type or not a known filetype: %s",
              qPrintable(to));
          continue;
        }

        qInfo() << "-- needle ----------";
        Media::print(search.needle);

        search = engine().query(search);

        Media::printGroup(search.matches);

        float vm, ws;
        Env::memoryUsage(vm, ws);
        qInfo("similar-to: %d items %d/%d MB", search.matches.count(),
              int(ws / 1024), int(vm / 1024));

        search.matches.prepend(search.needle);
        list.append(search.matches);
      } else {
        MediaGroup needles = selectPath(to);

        // todo: external dir search, workaround is adding to
        // index temporarily
        if (needles.count() <= 0)
          qWarning()
              << "nothing selected, external directory search unsupported";

        // this not the same as similar-in... which is a subset query
        QList<QFuture<MediaSearch>> work;
        for (const Media& m : needles) {
          search.needle = m;
          work.append(QtConcurrent::run(&engine(), &Engine::query, search));
        }

        int i = 1;
        for (auto& w : work) {
          try {
            w.waitForFinished();
          }
          catch(std::exception& e) {
            qCritical("exception: %s", e.what());
            w.waitForFinished();
          }

          search = w.result();

          // note: engine.query(db.similarTo) already filtered groups
          // only need to group list
          if (search.matches.count() > 0) {
            search.matches.prepend(search.needle);
            list.append(search.matches);
          }

          qInfo("similar-to:<PL> %d/%d", ++i, work.count());
        }
        engine().db->filterMatches(params, list);

        Media::sortGroupList(list, "path");
      }
      queryResult = list;
    } else if (arg == "-select-none") {
      selection.clear();
    } else if (arg == "-select-all") {
      selection.append(engine().db->mediaWithPathLike("%"));
    } else if (arg == "-select-id") {
      int id = intArg(nextArg());
      Media m = engine().db->mediaWithId(id);
      if (m.isValid() <= 0)
        qWarning("select-id: nothing found");
      else
        selection.append(m);
    } else if (arg == "-select-one") {
      Media m = engine().db->mediaWithPath(nextArg());
      if (!m.isValid())
        qWarning("select-one: invalid path, is it in the index?");
      else
        selection.append(m);
    } else if (arg == "-select-type") {
      int type = parseType(nextArg());
      selection.append(engine().db->mediaWithType(type));
    } else if (arg == "-select-path") {
      selection.append(selectPath(nextArg()));
    } else if (arg == "-select-errors") {
      QMutexLocker locker(Scanner::staticMutex());
      const auto errors = Scanner::errors();
      for (auto it = errors->begin(); it != errors->end(); ++it)
        if (!it.value().contains(Scanner::ErrorUnsupported)) {
          QString path = it.key();
          selection.append(Media(path));
        }
    } else if (arg == "-select-result") {
      for (auto& g : queryResult) selection.append(g);
      queryResult.clear();
    } else if (arg == "-select-sql") {
      selection.append(engine().db->mediaWithSql(nextArg()));
    } else if (arg == "-select-files") {
      if (args.count() < 1)
        qFatal("-select-files expects one or more arguments");

      while (args.count() > 0) {
        QString arg = args.front();
        if (arg.startsWith("-"))  // next switch
          break;
        args.pop_front();

        const QFileInfo info(arg);
        if (!info.exists()) {
          qWarning() << "select-files: file not found:" << arg;
          continue;
        }
        QString ext = info.suffix().toLower();

        if (engine().scanner->archiveTypes().contains(ext)) {
          // take the first item in the zip
          auto list = Media::listArchive(arg);
          for (auto& path : list)
            if (engine().scanner->imageTypes().contains(
                    QFileInfo(path).suffix().toLower())) {
              arg = path;
              ext = QFileInfo(arg).suffix();
              break;
            }
        }

        int type = 0;
        if (engine().scanner->imageTypes().contains(ext))
          type = Media::TypeImage;
        else if (engine().scanner->videoTypes().contains(ext))
          type = Media::TypeVideo;
        else
          qWarning() << "select-files: unknown file type:" << arg;

        if (type) selection.append(Media(arg, type));
      }
    } else if (arg == "-select-grid") {
      const Media grid(nextArg());
      QImage qImg = grid.loadImage();
      cv::Mat cvImg;
      QVector<QRect> rects;

      qImageToCvImg(qImg, cvImg);
      demosaic(cvImg, rects);

      MediaGroup g;
      int i = 0;
      for (const QRect& r : rects) {
        Media m;
        m.setPath(grid.path().split("/").last() + "@rect" + QString::number(i++));
        m.setImage(qImg.copy(r));
        g.append(m);
      }
      selection.append(g);
    } else if (arg == "-rename") {
      QString srcPat, dstPat, options;
      srcPat = nextArg();
      dstPat = nextArg();

      if (args.count() > 0) options = args.takeFirst();

      if (srcPat == "*") srcPat = "^.*$";

      const QRegularExpression re(srcPat);
      if (!re.isValid())
        qFatal("rename: <find> pattern <%s> is illegal regular expression: %s at offset %d",
               qPrintable(srcPat), qPrintable(re.errorString()), re.patternErrorOffset());

      int pad = int(log10(double(selection.count()))) + 1;
      int num = 1;

      bool findReplace = false;
      if (!dstPat.contains("#")) {
        findReplace = true;
        qInfo() << "rename: no captures in <replace> pattern, using substring find/replace";
      }

      for (int i = 1; i < re.captureCount(); ++i)
        if (!dstPat.contains("#" + QString::number(i)))
          qCritical("rename: capture #%d is discarded", i);

      for (int i = re.captureCount() + 1; i < re.captureCount() + 10; ++i)
        if (dstPat.contains("#" + QString::number(i)))
          qCritical("rename: capture reference (#%d) with no capture", i);

      QStringList newNames;
      MediaGroup toRename;
      for (auto& m : selection) {
        if (m.isArchived()) {
          qWarning() << "rename: cannot rename archived file:" << m.path();
          continue;
        }

        const bool matchPath = options.indexOf("p") >= 0;
        const QFileInfo info(m.path());
        QString oldName = info.completeBaseName();
        if (matchPath) {
          QStringList relParts = m.path().mid(engine().db->path().length()+1).split("/");
          relParts.removeLast();
          oldName = relParts.join("/") + "/" + oldName;
        }

        if (info.suffix().isEmpty()) {
          qWarning() << "rename: no file extension:" << m.path();
          continue;
        }

        QString newName;
        if (findReplace) {
          newName = oldName;
          newName = newName.replace(QRegularExpression(srcPat), dstPat);
          if (newName.contains("%1"))
            newName = newName.arg(num, pad, 10, QChar('0'));
          else if (newName == oldName) {
            if (options.indexOf("v") >= 0)
              qWarning("rename: <find> text (%s) doesn't match: <%s>",
                       qPrintable(srcPat), qPrintable(oldName));
            continue;
          }
        } else {
          newName = dstPat;

          QRegularExpressionMatch match = re.match(oldName);
          if (!match.hasMatch()) {
            if (options.indexOf("v") >= 0)
              qWarning("rename: <find> regexp <%s> does not match: <%s>",
                       qPrintable(srcPat), qPrintable(oldName));
            continue;
          }
          QStringList captured = match.capturedTexts();
          for (int i = 0; i < captured.size(); ++i) {
            QString placeholder = "#" + QString::number(i);
            newName = newName.replace(placeholder, captured[i]);
          }

          if (newName.contains("%1"))
            newName = newName.arg(num, pad, 10, QChar('0'));
        }


        {
          struct Replacement {
            int start, end;
            QString text;
          };
          QVector<Replacement> replacements;

          int funcOpen  = newName.indexOf("{");
          int funcClose = newName.indexOf("}", funcOpen+1);
          while (funcOpen >= 0 && (funcClose-funcOpen) > 1) {
            auto funcs = newName.mid(funcOpen+1, funcClose-funcOpen-1).split(":");
            qWarning() << "rename: function:" << funcs;
            if (funcs.count() > 0) {
              QVariant result = funcs[0];
              funcs.removeFirst();

              while (funcs.count() > 0) {
                result = (Media::unaryFunc(funcs[0]))(result);
                funcs.removeFirst();
              }
              qDebug() << result;
              replacements.append( {funcOpen, funcClose+1, result.toString() });
            }
            else {
              qFatal("rename: function missing argument");
            }

            funcOpen  = newName.indexOf("{", funcClose+1);
            funcClose = newName.indexOf("}", funcOpen+1);
          }
          while (!replacements.empty()) {
            auto r = replacements.takeLast();
            newName = newName.replace(r.start, r.end-r.start, r.text);
          }
        }

        newName += "." + info.suffix();
        if (newName.contains("/")) // fixme: add proper set
          qFatal("rename: new filename contains illegal characters %s -> <%s>",
                 qPrintable(m.path()), qPrintable(newName));

        const QString newPath = info.dir().absolutePath() + "/" + newName;

        if (newNames.contains(newPath))
          qWarning("rename: collision: %s,%s => %s",
                 qPrintable(toRename[newNames.indexOf(newPath)].path()),
                 qPrintable(oldName), qPrintable(newName));
        else if (info.dir().exists(newName))
          qWarning("rename: new name will overwrite: %s -> %s",
                 qPrintable(m.path()), qPrintable(newName));
        else {
          newNames.append(newPath);
          toRename.append(m);
          num++;
        }
      }

      Q_ASSERT(newNames.count() == toRename.count());

      for (int i = 0; i < toRename.count(); ++i) {
        auto m = toRename[i];
        qDebug() << m.path() << "->" << newNames[i];
        if (options.indexOf("x") < 0) continue;
        if (!engine().db->rename(m, newNames[i].split("/").back()))
          qFatal("rename failed, update index?");
      }

      if (options.indexOf("x") >= 0)
        qInfo() << "renamed" << toRename.count() <<
          ", skipped" << selection.count()-toRename.count();

      selection.clear();
    } else if (arg == "-move") {
      Q_ASSERT(args.count() > 0);
      const QString dstDir = absolutePath(args.front());
      args.pop_front();

      qInfo() << "moving " << selection.count() << "items to:" << dstDir;
      for (auto& m : selection)
        if (!engine().db->move(m, dstDir)) break;
    } else if (arg == "-with" || arg == "-without") {
      const QString key = nextArg();
      const QString value = nextArg();

      const auto getValue = Media::propertyFunc(key);

      bool without = arg == "-without";

      const Comparator cmp(value);

      if (selection.count() > 0) {
        auto future = QtConcurrent::map(selection, [&](Media& m) {
          if (without ^ cmp.compareTo(getValue(m)))
            m.setAttribute("filter", ":yes");
        });
        while (future.isRunning()) {
          qInfo("filtering selection:<PL> %d/%d", future.progressValue(),
                 future.progressMaximum());
          QThread::msleep(100);
        }

        MediaGroup tmp;
        for (Media& m : selection) {
          if (m.attributes()["filter"] == ":yes") {
            m.setAttribute("filter", key + " " + value);
            tmp.append(m);
            // qDebug() << m.attributes();
          }
        }
        qInfo() << "filter:" << tmp.count() << "/" << selection.count()
                << "items";
        selection = tmp;
      }

      if (queryResult.count() > 0) {
        auto future = QtConcurrent::map(queryResult, [&](MediaGroup& g) {
          for (auto& m : g)
            if (without ^ cmp.compareTo(getValue(m)))
              m.setAttribute("filter", ":yes");
        });

        while (future.isRunning()) {
          qInfo("filtering result:<PL> %d/%d", future.progressValue(),
                 future.progressMaximum());
          QThread::msleep(100);
        }

        MediaGroupList tmp;
        for (auto& g : queryResult) {
          MediaGroup filtered;
          for (auto& m : g)
            if (m.attributes()["filter"] == ":yes") {
              m.setAttribute("filter", key + " " + value);
              filtered.append(m);
            }

          if (!filtered.empty()) tmp.append(filtered);
        }
        queryResult = tmp;
      }
    } else if (arg == "-first") {
      for (auto& g : queryResult)
        if (g.count() > 0) g = {g[0]};
      if (selection.count() > 0) selection = {selection[0]};
    } else if (arg == "-chop") {
      for (auto& g : queryResult)
        if (g.count() > 0) g.removeFirst();
      if (selection.count() > 0) selection.removeFirst();
    } else if (arg == "-first-sibling") {
      auto fn = [=](MediaGroup& g) {
        QSet<QString> parents;
        MediaGroup filtered;
        for (auto& m : g) {
          auto p = m.parentPath();
          if (parents.contains(p)) continue;
          filtered.append(m);
          parents.insert(p);
        }
        return filtered;
      };

      if (queryResult.count() > 0)
        for (auto& g : queryResult) g = fn(g);
      else if (selection.count() > 0)
        selection = fn(selection);
    } else if (arg == "-sort" || arg == "-sort-rev") {
      const QString sortKey = nextArg();
      // pre-compute property values and cache them
      auto getValue = Media::propertyFunc(sortKey);
      auto future = QtConcurrent::map(selection, getValue);
      future.waitForFinished();
      Media::sortGroup(selection, sortKey, arg == "-sort-rev");
    } else if (arg == "-group-by") {
      const QString arg = nextArg();
      auto getProperty = Media::propertyFunc(arg);

      // mt to fetch the value
      QVector<QFuture<QVariant>> work;
      for (auto& m : selection)
        work.append(QtConcurrent::run([=]() { return getProperty(m); }));

      QHash<QString, MediaGroup> groups;
      for (auto& m : selection) {
        auto f = work.front();
        f.waitForFinished();
        work.pop_front();
        qInfo("group-by:<PL> %d/%d", selection.count() - work.count(),
               selection.count());

        const QString key = f.result().toString();

        // display key for the group in views
        m.setAttribute("group", arg + " == " + key);

        auto it = groups.find(key);
        if (it != groups.end())
          it->append(m);
        else
          groups.insert(key, MediaGroup({m}));
      }

      selection.clear();
      queryResult = groups.values().toVector();
    } else if (arg == "-sort-similar") {
      MediaGroup sorted;
      SearchParams sp = params;
      sp.set = selection;
      sp.inSet = true;
      // sp.maxMatches = 2;

      const int lookBehind = 5;
      Q_ASSERT(selection.count() > 0);

      QMap<QString, Media> unsorted;

      for (auto& m : selection) unsorted.insert(m.path(), m);

      sorted.append(selection.front());
      unsorted.remove(selection.front().path());

      for (int i = 0; i < selection.count() - 1; ++i) {
        // search won't necessarily find anything,
        // improve chances by looking behind up to 5 items
        for (int j = 0; j < qMin(lookBehind, sorted.count()); j++) {
          MediaSearch search;
          search.params = sp;
          search.needle = sorted[sorted.count() - 1 - j];

          // set must also contain needle or else we can't search for it
          MediaGroup querySet = unsorted.values().toVector();
          querySet.append(search.needle);

          search.params.set = querySet; // subset query
          search.params.inSet = true;

          search = engine().query(search);

          if (search.matches.count() > 0) {
            Media& match = search.matches[0];
            // sorted.append(match);
            sorted.insert(sorted.count() - j, match);
            unsorted.remove(match.path());
            break;
          }
        }

        // if we did not find anything, what now?
      }

      int missed = selection.count() - sorted.count();
      if (missed != 0)
        qWarning() << "sort-similar: " << missed
                   << " items were not found and dropped, use fuzzier search";
      selection = sorted;
    } else if (arg == "-merge") {
      MediaGroup setA = selectPath(nextArg());
      MediaGroup setB = selectPath(nextArg());

      // todo: generalized multisearch
      auto multiSearch = [](const Media& needle, const SearchParams& params) {
        MediaSearch search;
        search.params = params;
        search.needle = needle;
        search.params.maxMatches = 2;

        // if match.score is >= threshold, use the next algorithm
        int thresholds[] = {
            12,
            1000,
            1000,
            INT_MAX,
        };

        bool ok = false;
        do {
          search = engine().query(search);
          // engine().db->filterMatch(params, search.matches);

          ok = search.matches.count() > 0;
          ok = ok && search.matches[0].score() < thresholds[search.params.algo];

          if (!ok) search.params.algo++;
          if (search.params.algo > 3) break;
        } while (!ok);
        if (!ok) search.matches.clear();

        return search;
      };

      QVector<QFuture<MediaSearch>> work;

      SearchParams multiParams = params;
      multiParams.set = setA;
      multiParams.inSet = true;

      for (const auto& b : setB) {
        multiParams.set.append(b);
        work.append(QtConcurrent::run(multiSearch, b, multiParams));
      }

      for (auto& wi : work) {
        wi.waitForFinished();
        MediaSearch search = wi.result();
        if (search.matches.count() <= 0) {
          qCritical() << "no match:" << search.needle.path();
          continue;
        }

        // search.matches.prepend(search.needle);
        // MediaBrowser::show({search.matches}, params);

        int pos = Media::indexInGroupByPath(setA, search.matches.at(0).path());
        Q_ASSERT(pos >= 0 && pos < setA.length());

        // we found the closest match, should it go before or after?
        if (pos > 0 && pos < setA.length() - 1) {
          int before = hamm64(search.needle.dctHash(), setA[pos - 1].dctHash());
          int after = hamm64(search.needle.dctHash(), setA[pos + 1].dctHash());

          if (after < before) pos++;
        }
        setA.insert(pos, search.needle);
      }
      selection = setA;
    } else if (arg == "-sets") {
      showMode = MediaBrowser::ShowPairs;
    } else if (arg == "-folders") {
      showMode = MediaBrowser::ShowFolders;
    } else if (arg == "-exit-on-select") {
      selectMode = MediaBrowser::SelectExitCode;
    } else if (arg == "-max-per-page") {
      maxPerPage = intArg(nextArg());
    } else if (arg == "-show") {
      if (!queryResult.isEmpty())
        MediaBrowser::show(queryResult, params, showMode);
      else {
        // note: cannot sort since it fucks up -sort-xxx feature
        // Media::sortGroup(selection, "path");

        qDebug("make groups of %d", maxPerPage);
        MediaGroupList list;
        MediaGroup group;
        QString parent =
            selection.count() > 0 ? selection[0].parentPath() : QString();
        for (const Media& m : selection) {
          bool closeGroup = false;

          // split on parent path if using folder view
          // show each video as a separate thumbnail
          if (showMode == MediaBrowser::ShowFolders) {
            if (parent == m.parentPath() && m.type() != Media::TypeVideo)
              group.append(m);
            else {
              closeGroup = true;
              parent = m.parentPath();
            }
          } else
            group.append(m);

          if (group.count() >= maxPerPage || closeGroup) {
            if (group.count() > 0) list.append(group);
            group.clear();
            if (closeGroup) group.append(m);
          }
        }

        if (group.count() > 0) list.append(group);

        int pos = 0;
        for (auto& g : list)
          for (auto& m : g)
            m.setPosition(pos++);

        qDebug("show browser: mode=%d groups=%d", showMode, list.count());
        int status = MediaBrowser::show(list, params, showMode, selectMode);
        if (selectMode == MediaBrowser::SelectExitCode) return status - 1;
      }
    } else if (arg == "-qualityscore") {
      IndexResult result = engine().scanner->processImageFile(nextArg());
      if (result.ok) {
        QVector<QImage> visuals;
        qualityScore(result.media, &visuals);
        //selection.append(result.media);
        queryResult.append({result.media});
        for (auto& img : visuals) {
          Media m(img);
          m.setPath(img.text("description"));
          //selection.append(m);
          queryResult.append({m});
        }
      }
    } else if (arg == "-updatemd5") {
      // deprecated: for old indexes that stored sparse md5, update to full md5
      for (const Media& m : selection) {
        Q_ASSERT(m.type() == Media::TypeVideo);
        {
          QFile f(m.path());
          if (!f.open(QFile::ReadOnly))
            qFatal("failed to open: %s", qPrintable(m.path()));
          if (m.md5() != sparseMd5(f)) {
            qCritical()
                << "updateMd5: no update since hash could be the new version"
                << m.path() << m.md5();
            continue;
          }
        }
        QString hash = Scanner::hash(m.path(), m.type());
        Media copy = m;
        if (!engine().db->setMd5(copy, hash) || copy.md5() != hash) return -1;
        qInfo() << "updateMd5" << m.path() << m.md5() << hash;
      }
    } else if (arg == "-jpeg-repair-script") {
      jpegFixPath = nextArg();
    } else if (arg == "-verify") {
      // note: hashes will not match when files get overwritten (rename tool
      // bypassed)
      if (indexParams.indexThreads > 0)
        QThreadPool::globalInstance()->setMaxThreadCount(
            indexParams.indexThreads);

      QAtomicInt okCount;
      QAtomicInteger<qint64> totalBytesRead;
      qint64 startTime = QDateTime::currentMSecsSinceEpoch();

      auto hashFunc = [&okCount,&totalBytesRead](const Media& m) {
        qint64 bytesRead = 0;
        QString hash = Scanner::hash(m.path(), m.type(), &bytesRead);
        bool match = hash == m.md5();
        // qDebug() << m.path();
        if (!match)
          qCritical() << "file hash changed:" << m.path().mid(engine().db->path().length()+1)
                      << "current:" << hash << "stored:" << m.md5();
        else
          okCount.ref();

        totalBytesRead += bytesRead;
      };

      // to avoid thrashing, large files are hashed sequentially
      // todo: IndexParams
      const qint64 largeFileSize = 16 * 1024 * 1024;

      for (const auto& m : selection)
        if (!m.isArchived() && QFileInfo(m.path()).size() >= largeFileSize)
          hashFunc(m);

      QFuture<void> f = QtConcurrent::map(
          selection.constBegin(), selection.constEnd(),
          [&hashFunc](const Media& m) {
            if (m.isArchived() || QFileInfo(m.path()).size() < largeFileSize)
              hashFunc(m);
          });
      f.waitForFinished();

      // since every file was touched, good time to
      // do repairs and other maintenance
      {
        QMutexLocker locker(Scanner::staticMutex());
        const auto errors = Scanner::errors();
        for (auto it = errors->begin(); it != errors->end(); ++it)
          if (it.value().contains(Scanner::ErrorJpegTruncated)) {
            QString path = it.key();
            if (!Media(path).isArchived()) {
              path = path.replace("\"", "\\\"");
              QString cmd = QString("%s \"%1\"").arg(jpegFixPath).arg(path);
              if (0 != system(qPrintable(cmd)))
                qWarning() << "jpeg repair script failed";
            }
          }
      }

      int numOk = okCount.loadRelaxed();
      qint64 endTime = QDateTime::currentMSecsSinceEpoch();
      qint64 mb = totalBytesRead.loadRelaxed() / 1024 / 1024;
      float hashRate = 1000.0f * mb / (endTime-startTime);

      qInfo() << "verified" << numOk << "/" << selection.count() << "," << mb << "MB," << hashRate << "MB/s";
      int status = 0;
      if (numOk == selection.count()) status = 1;
      exit(status);
    } else if (arg == "-test-csv") {
      QFile csv(nextArg());
      csv.open(QFile::ReadOnly);
      QByteArray line;
      int numImages, numFound;
      numImages = numFound = 0;
      while ("" != (line = csv.readLine())) {
        const QStringList tmp = QString(line).split(";");

        if (tmp.size() < 2) continue;

        numImages++;
        QString src = tmp[0];
        QString dst = tmp[1];
        src.replace("\"", "");
        dst.replace("\"", "");

        qInfo() << "testing " << src << "=>" << dst;

        Media m(src);
        MediaGroup results = engine().db->similarTo(m, params);

        Media found;
        int i;
        for (i = 0; i < results.size(); i++)
          if (results[i].path() == dst) {
            found = results[i];
            numFound++;
            break;
          }

        m.recordMatch(found, i + 1, results.size());
      }
      qInfo() << "accuracy:" << (numFound * 100.0 / numImages) << "%";
    } else if (arg == "-vacuum") {
      engine().db->vacuum();
    } else if (arg == "-test-add-video") {
      IndexResult result = engine().scanner->processVideoFile(nextArg());
      if (result.ok) engine().db->add({result.media});
    } else if (arg == "-compare-videos") {
      Media left(nextArg(), Media::TypeVideo);
      Media right = left;
      if (args.count() > 0) right = Media(nextArg(), Media::TypeVideo);
      VideoCompareWidget v(left, right);
      v.show();
      v.activateWindow();
      app->exec();
    } else if (arg == "-view-image") {
      Media m(nextArg(), Media::TypeImage);
      MediaGroupList l{{m}};
      MediaBrowser::show(l, params, showMode);
    } else if (arg == "-test-video-decoder") {
      QString path = nextArg();
      VideoContext video;
      VideoContext::DecodeOptions opt;

      opt.rgb = 0;
      opt.gpu = indexParams.useHardwareDec;
      opt.threads = indexParams.decoderThreads;
      opt.maxH = 128;
      opt.maxW = 128;
      Q_ASSERT(0 == video.open(path, opt));

      int numFrames = 0;
      qint64 then = QDateTime::currentMSecsSinceEpoch();

      while (video.decodeFrame()) {
        numFrames++;
        if (numFrames > 100) {
          qint64 now = QDateTime::currentMSecsSinceEpoch();
          qInfo() << numFrames * 1000.0f / (now - then) << "frames/second";
          then = now;
          numFrames = 0;
        }
      }
    } else if (arg == "-test-video") {
      VideoContext vc;
      VideoContext::DecodeOptions opt;
      // settings used by indexer, maybe higher hit rate
      opt.maxW = 128;
      opt.maxH = 128;
      opt.rgb = 0;
      opt.gpu = indexParams.useHardwareDec;
      opt.threads = indexParams.decoderThreads;

      // settings used in practical use
      // opt.rgb = 1;

      // arg must be file we know to be in the db already
      const QString path = nextArg();

      if (0 != vc.open(path, opt)) {
        qWarning("failed to open");
        continue;
      }

//#define SHOWFRAMES
//#define DECODETEST
#ifdef SHOWFRAMES
      QLabel* label = nullptr;
      QImage qImg;
#endif
#ifdef DECODETEST
      int numFrames = 0;

      while (vc.nextFrame(qImg)) {
        numFrames++;
        printf("%d\r", numFrames);
        fflush(stdout);
#ifdef SHOWFRAMES
        if (!label) label = new QLabel;
        label->setPixmap(QPixmap::fromImage(qImg));
        label->show();
        qApp->processEvents();
#endif
      }
      printf("videotest: video contains %d frames\n", numFrames);

      continue;
#endif

#define USE_THREADS 1

#if USE_THREADS
      QList<QFuture<MediaSearch>> work;
#endif
      QString absPath = QFileInfo(path).absoluteFilePath();
      QString results;
      int sumDistance = 0, minDistance = INT_MAX, maxDistance = INT_MIN;
      cv::Mat img;
      int srcFrame = -1;
      QVector<int> rangeError;
      while (vc.nextFrame(img)) {
        srcFrame++;
        {
#ifdef SHOWFRAMES
          cvImgToQImage(img, qImg);
          if (!label) label = new QLabel;
          label->setPixmap(QPixmap::fromImage(qImg));
          label->show();
          qApp->processEvents();
#endif
        }

#if USE_THREADS
        cv::Mat imgCopy = img.clone();
        work.append(QtConcurrent::run([srcFrame, imgCopy, params]() {
          cv::Mat img = imgCopy;
          autocrop(img, 20);  // same as indexer
          uint64_t hash = dctHash64(img);

          Media m("", Media::TypeImage, img.cols, img.rows, "md5", hash);
          MediaSearch search;
          m.setMatchRange({srcFrame, -1, 0});
          search.needle = m;
          search.params = params;

          return engine().query(search);
        }));
#else

        autocrop(img, 20);
        uint64_t hash = phash64(img);

        Media m("", Media::TypeImage, img.cols, img.rows, "md5", hash);
        MediaSearch search;
        search.needle = m;
        search.params = params;

        search = engine().query(search);
#endif

#if USE_THREADS
        while (work.count() > 0) {
          auto& future = work[0];
          if (!future.isFinished()) break;

          MediaSearch search = future.result();
          work.removeFirst();
#endif
          char status = 'n';
          int matchIndex = -1;
          if (search.matches.count() > 0) {
            matchIndex = Media::indexInGroupByPath(search.matches, absPath);
            if (matchIndex == 0)
              status = 'Y';
            else if (matchIndex > 0)
              status = 'p';
            else
              status = '0';
          }
          results += status;

          if (matchIndex >= 0) {
            auto range = search.matches[matchIndex].matchRange();
            int distance = abs(range.srcIn - range.dstIn);
            sumDistance += distance;
            rangeError.append(distance);
            minDistance = qMin(minDistance, distance);
            maxDistance = qMax(maxDistance, distance);
            // printf("%d %d\n", range.srcIn, range.dstIn);
          }

          qFlushOutput();
          printf("%c", status);
          //fflush(stdout);
#if USE_THREADS
        }
#endif

      }  // while nextFrame

      // adjust for vpad
      results = results.mid(
          params.skipFramesIn,
          results.length() - params.skipFramesIn - params.skipFramesOut);

      int found, bad, poor, none;
      found = bad = poor = none = 0;

      for (int i = 0; i < results.length(); i++)
        switch (results[i].toLatin1()) {
          case 'Y':
            found++;
            break;
          case 'p':
            poor++;
            break;
          case '0':
            bad++;
            break;
          default:
            none++;
        }

      int frames = results.length();

      std::sort(rangeError.begin(), rangeError.end());

      qFlushOutput();

      printf("\nframes=%d found=%.3f%% poor=%.3f%% bad=%.3f%% none=%.3f%%\n",
             frames, found * 100.0 / frames, poor * 100.0 / frames,
             bad * 100.0 / frames, none * 100.0 / frames);
      printf("range error (frames): mean=%.3f, min=%d, max=%d, median=%d\n\n",
             double(sumDistance) / (found + poor), minDistance, maxDistance,
             rangeError[rangeError.length() / 2]);
    } else if (arg == "-test-update") {
      QDialog* d = new QDialog();
      QPushButton* b = new QPushButton(d);
      QPushButton* c = new QPushButton(d);
      QPushButton* f = new QPushButton(d);

      b->setText("Start Update");
      c->setText("Stop Update");
      f->setText("Finish Update");

      QObject::connect(engine().scanner, &Scanner::scanCompleted, [=] {
        qDebug() << "\n\nscan completed";
        b->setText("Start Update");
        c->setText("Stop Update");
        f->setText("Finish Update");
      });

      QObject::connect(b, &QPushButton::pressed, [=] {
        b->setText("Updating...");
        engine().update();
      });

      QObject::connect(c, &QPushButton::pressed, [=] {
        qDebug() << "\n\nstop update";
        c->setText("Stopping...");
        engine().stopUpdate();
      });

      QObject::connect(f, &QPushButton::pressed, [=] {
        qDebug() << "\n\nfinish update";
        f->setText("Finishing...");
        engine().scanner->finish();
      });

      QHBoxLayout* l = new QHBoxLayout(d);
      l->addWidget(b);
      l->addWidget(c);
      l->addWidget(f);
      d->exec();
      delete d;
    } else {
      qCritical("unknown argument=%s", qPrintable(arg));
#ifdef Q_OS_WIN
      if (arg == "-p" || arg == "-i")
        qWarning() << "in PowerShell you must use -p: / -i: ";
#endif
      return 1;
    }
  }

  return 0;
}
