
#include <QtTest/QtTest>

#include "media.h"
#include "scanner.h"

class TestScanner : public QObject {
  Q_OBJECT

 private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();

  void init();

  void testDefaults();
  void testEmptyDir();
  void test200FilesDir();
  void testDestructor();
  void testSkipListPresent();
  void testSkipListMissing();
  void test1VideoDir();
  void testCorruptedFiles();

  void mediaProcessed(const Media& m);

 private:
  QString _dataDir;
  QSet<QString> _filesAdded;
  QSet<QString> _filesRemoved;
};

void TestScanner::initTestCase() {
  _dataDir = getenv("TEST_DATA_DIR");
  if (_dataDir.isEmpty()) qFatal("TEST_DATA_DIR environment is not set");
}

void TestScanner::cleanupTestCase() {}

void TestScanner::init() { _filesAdded.clear(); }

void TestScanner::mediaProcessed(const Media& m) {
  //qDebug() << "mediaProcesssed" << m.path();
  _filesAdded.insert(m.path());
}

void TestScanner::testDefaults() {
  Scanner scanner;
  QVERIFY(scanner.imageTypes().contains("jpg"));
  QVERIFY(scanner.videoTypes().contains("mp4"));
}

void TestScanner::testEmptyDir() {
  // test empty dir scan works and has 0 results
  QCOMPARE(_filesAdded.count(), 0);
  Scanner scanner;
  QSet<QString> skip;
  connect(&scanner, &Scanner::mediaProcessed, this,
          &TestScanner::mediaProcessed);
  scanner.scanDirectory(_dataDir + "/scanner/emptydir", skip);
  scanner.finish();
  QCOMPARE(_filesAdded.count(), 0);
}

void TestScanner::test200FilesDir() {
  // test the slot was called 200 times
  QCOMPARE(_filesAdded.count(), 0);
  Scanner scanner;
  QSet<QString> skip;
  connect(&scanner, &Scanner::mediaProcessed, this,
          &TestScanner::mediaProcessed);
  scanner.scanDirectory(_dataDir + "/scanner/200images", skip);
  scanner.finish();
  QCOMPARE(_filesAdded.count(), 200);
}

void TestScanner::testDestructor() {
  // test the destructor blocks to flush the work queue
  QCOMPARE(_filesAdded.count(), 0);

  {
    Scanner scanner;
    QSet<QString> skip;
    connect(&scanner, &Scanner::mediaProcessed, this,
            &TestScanner::mediaProcessed);
    scanner.scanDirectory(_dataDir + "/scanner/200images", skip);

    // implicit flush; since we didn't enter event loop, no threads
    // started, nothing should be returned
  }

  QCOMPARE(_filesAdded.count(), 0);
}

void TestScanner::testSkipListPresent() {
  // test that scanDir ignores any path in the list and removes it from the list
  QCOMPARE(_filesAdded.count(), 0);

  {
    Scanner scanner;
    QSet<QString> skip;
    connect(&scanner, &Scanner::mediaProcessed, this,
            &TestScanner::mediaProcessed);
    scanner.scanDirectory(_dataDir + "/scanner/200images", skip);
    scanner.finish();
  }

  QCOMPARE(_filesAdded.count(), 200);

  // add first 10 files to skip list
  QSet<QString> skip;
  for (const QString& path : _filesAdded) {
    if (skip.count() < 10) skip.insert(path);
  }

  QCOMPARE(skip.count(), 10);

  // run it again, this time we get 190 files
  _filesAdded.clear();

  {
    Scanner scanner;
    connect(&scanner, &Scanner::mediaProcessed, this,
            &TestScanner::mediaProcessed);
    // scanner.setRecursive(true);
    scanner.scanDirectory(_dataDir + "/scanner/200images", skip);
    scanner.finish();
  }

  // skip list is zero since all files existed
  QCOMPARE(skip.count(), 0);

  // count is less the number we skipped
  QCOMPARE(_filesAdded.count(), 190);
}

void TestScanner::testSkipListMissing() {
  // test skip list unmodified if files do not exist

  QSet<QString> skip;

  skip.insert("bogus1.jpg");
  skip.insert("dummy/bogus2.jpg");

  QCOMPARE(skip.count(), 2);

  {
    Scanner scanner;
    scanner.scanDirectory(_dataDir + "/scanner/emptydir", skip);
    scanner.finish();
  }

  QCOMPARE(skip.count(), 2);
}

void TestScanner::test1VideoDir() {
  QCOMPARE(_filesAdded.count(), 0);

  {
    Scanner scanner;
    QSet<QString> skip;
    connect(&scanner, &Scanner::mediaProcessed, this,
            &TestScanner::mediaProcessed);
    scanner.scanDirectory(_dataDir + "/scanner/1video", skip);
    scanner.finish();
  }

  QCOMPARE(_filesAdded.count(), 1);
}

void TestScanner::testCorruptedFiles() {
  QCOMPARE(_filesAdded.count(), 0);

  {
    Scanner scanner;
    QSet<QString> skip;
    connect(&scanner, &Scanner::mediaProcessed, this,
            &TestScanner::mediaProcessed);
    // set minimum 0 so we try to read some empty files
    IndexParams params;
    params.minFileSize = 0;
    scanner.setIndexParams(params);
    scanner.scanDirectory(_dataDir + "/scanner/corrupt", skip);
    scanner.finish();
  }

  // there is one truncated jpeg which we will allow to pass
  QCOMPARE(_filesAdded.count(), 1);
}

QTEST_MAIN(TestScanner)
#include "testscanner.moc"
