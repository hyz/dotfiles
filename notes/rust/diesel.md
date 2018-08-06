
    cargo install -f diesel_cli --no-default-features --features postgres
    diesel --database-url 'postgres:///wood' print-schema

    diesel setup
    diesel migration generate warelis
    diesel migration run
    diesel migration redo
    diesel print-schema
    diesel print-schema > src/schema.rs

### http://diesel.rs/guides/getting-started/

### https://stackoverflow.com/questions/27037990/connecting-to-postgres-via-database-url-and-unix-socket-in-rails

    postgres://username@/dbname
    postgres:///dbname

    ...
    postgresql:///dbname?host=/var/lib/postgresql

