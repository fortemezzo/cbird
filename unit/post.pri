
# find .h and .cpp that go with a file prefix,
# then make relative to the unit test dir
# note: path relative to CWD of this .qm file!
for (BASE, FILES) {

    LIST=../$${BASE}.cpp $${BASE}.cpp

    for (FILE, LIST) {
        exists($$FILE) {
            SOURCES += $$FILE
        }
    }

    LIST=../$${BASE}.h $${BASE}.h
    for (FILE, LIST) {
        exists($$FILE) {
            HEADERS += $$FILE
        }
    }
}

