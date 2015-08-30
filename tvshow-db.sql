PRAGMA journal_mode = PERSIST;

CREATE TABLE show_table
(
        id              INTEGER PRIMARY KEY,
        channel_name            TEXT(1024),
        title	TEXT(1024),
        date       TEXT(1024),
        wday	INTEGER,
        time    TEXT(1024),
        thumb	TEXT(1024),
        fav		INTEGER
);