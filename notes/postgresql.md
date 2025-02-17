
    sudo systemctl stop postgresql.service
    sudo -iu postgres -- initdb --locale=en_US.UTF-8 -E UTF8 --auth-local trust -D /var/lib/postgres/data

    sudo -iu postgres -- dropdb --if-exists $USER
    sudo -iu postgres -- dropuser --if-exists $USER
    sudo -iu postgres -- createuser --interactive $USER
    sudo -iu postgres -- createdb -O $USER $USER
    sudo -iu postgres -- psql -c "CREATE DATABASE $USER OWNER $USER"

    psql -c "CREATE USER zero1 WITH PASSWORD '`pass anywhere`'"
    psql -c "CREATE DATABASE xapp OWNER zero1"
    psql -c "GRANT ALL PRIVILEGES ON DATABASE xapp TO zero1"
    psql -U zero1 xapp -f xapp.sql

    psql <<< "SELECT version()"
    psql -c \\conninfo
    psql -c \\? variables

    psql -l # psql -c \\l # psql <<< \\list # psql -c \\l+

    cat >> /tmp/mytab1.sql <<_EoF
    CREATE TABLE mytab1(
       ID INT PRIMARY KEY     NOT NULL,
       NAME           TEXT    NOT NULL,
       AGE            INT     NOT NULL,
       ADDRESS        CHAR(50),
       SALARY         REAL
    );
_EoF

    psql -d mydb1 -f /tmp/mytab1.sql
    psql -d mydb1 -c \\dt
    psql -d mydb1 -c \\d\ mytab1

    psql postgresql://$USER@localhost/mydb1

### new refresh postgres/data

    systemctl stop postgresql.service
    mv /var/lib/postgres/data /var/lib/postgres/data`date +%y%b%d`
    mkdir /var/lib/postgres/data
    chown postgres:postgres /var/lib/postgres/data
    sudo -iu postgres -- createuser --interactive $USER
    #just # psql -c "CREATE USER zero1 WITH PASSWORD '`pass anywhere`'"
    ...

### https://stackoverflow.com/questions/3327312/drop-all-tables-in-postgresql

    psql mydb -c "DROP SCHEMA public CASCADE;"
    psql mydb -c "CREATE SCHEMA public;"
    psql mydb -c "DROP SCHEMA public CASCADE; CREATE SCHEMA public;"

    psql mydb -c "GRANT ALL ON SCHEMA public TO postgres;"
    psql mydb -c "GRANT ALL ON SCHEMA public TO public;"

    DROP DATABASE realworld;
    CREATE DATABASE realworld WITH OWNER = postgres ENCODING = 'UTF8' TABLESPACE = pg_default LC_COLLATE = 'en_US.UTF-8' LC_CTYPE = 'en_US.UTF-8' CONNECTION LIMIT = -1;
    CREATE DATABASE realworld WITH TEMPLATE mydb OWNER dbuser;

    host=/run/postgresql user=root dbname=mydb sslmode=disable # UNIX socket
    postgres+unix:/run/postgresql/mydb

### sqlx/sqlx-cli/README.md

    sqlx database create
    sqlx database drop

    sqlx migrate run

    sqlx migrate add <name>

    cargo sqlx prepare


### https://wiki.archlinux.org/index.php/PostgreSQL#Upgrading_PostgreSQL
    
    cd /var/lib/postgres
    sudo mv data olddata
    sudo mkdir data tmp
    sudo chown postgres:postgres data tmp

    sudo -iu postgres -- initdb ......
    ...

### db admin, backup/restore

    pg_dump --data-only --column-inserts `date +decor%y%m%d` -t entitys

    sudo -iu postgres -- pg_dump --column-inserts --data-only mydb -t mytab > /tmp/mytab

    pg_dump mydb --schema-only |tee mydb.sql > db/mydb.sql.$(date +%m%d%H%M)
    vim mydb.sql

    pg_dump --schema-only mydb -t tab1 |tee tab1.sql > db/tab1.sql.$(date +%m%d%H%M)
    psql mydb -c '\d+ tab1'

    pg_restore -d mydb /tmp/qz2017020901.backup

    pg_dump mydb --schema-only |tee dump/mydb.schema-only.$(date +%m%d%H%M) > a.dump

### user/password manager

    postgres$ createuser --interactive

    psql mydb -c 'SELECT rolname, rolsuper FROM pg_roles'
    psql mydb -c '\du'

    # ALTER USER myuser WITH PASSWORD 'abc123';
    # ALTER USER myuser WITH ENCRYPTED PASSWORD 'abc123';
    # ALTER USER myuser WITH SUPERUSER;

### https://www.mkyong.com/database/backup-restore-database-in-postgresql-pg_dumppg_restore/

    pg_dump    -v -i -h localhost -p 5432 -U postgres -F c -b -v -f mydb.dump mydb
    pg_restore -v -i -h localhost -p 5432 -U postgres -d mydb mydb.dump

### https://dba.stackexchange.com/questions/83164/remove-password-requirement-for-user-postgres?rq=1

pg_hba.conf controls the authentication method. If you want to request a password, use md5 authentication. If you want to allow login with no password to anyone, use trust. If you want to require the same username in the operating system as in PostgreSQL, use peer (UNIX, only for local connections) or sspi (Windows).

If there's a password set, but pg_hba.conf doesn't tell PostgreSQL to ask for it, the password is ignored.

    vim /etc/postgresql/9.5/main/pg_hba.conf

### https://dba.stackexchange.com/questions/14740/how-to-use-psql-with-no-password-prompt

You have four choices regarding the password prompt:

- set the PGPASSWORD environment variable. For details see the manual:
    http://www.postgresql.org/docs/current/static/libpq-envars.html
- use a .pgpass file to store the password. For details see the manual:
    http://www.postgresql.org/docs/current/static/libpq-pgpass.html
- use "trust authentication" for that specific user:
    http://www.postgresql.org/docs/current/static/auth-methods.html#AUTH-TRUST
- use a connection URI that contains everything:
    http://www.postgresql.org/docs/current/static/libpq-connect.html#AEN42532

### https://postgrest.com/en/v4.3/tutorials/tut1.html#step-1-add-a-trusted-user

### https://wiki.archlinux.org/index.php/PostgreSQL

    psql
    vim /var/lib/postgres/data/pg_hba.conf
    vim /var/lib/postgres/data/postgresql.conf # listen_addresses

### https://www.postgresql.org/docs/current/static/

- https://www.postgresql.org/docs/current/static/app-psql.html

    psql mydb -c '\?'
    psql mydb -c '\l'
    psql mydb -c '\du'
    psql mydb -c '\dt'
    psql mydb -c '\dv'
    psql mydb -c '\d table_A'
    psql mydb -c '\h SELECT'
    psql mydb -c '\a' -c "SELECT * FROM table_A"

- https://www.postgresql.org/docs/current/static/app-pgdump.html
- https://www.postgresql.org/docs/current/static/app-pgrestore.html

    pg_dump.exe --host localhost --port 5432 --username "postgres" --no-password --verbose -F t --file "/tmp/mydatabase.sql" "mydatabase"

- https://stackoverflow.com/questions/109325/postgresql-describe-table
    psql mydb -c '\d+ table_A'

- https://astaxie.gitbooks.io/build-web-application-with-golang/content/en/05.4.html
    PostgreSQL

- http://godoc.org/github.com/lib/pq

### https://wiki.postgresql.org/wiki/Converting_from_other_Databases_to_PostgreSQL#MySQL
- https://github.com/dimitri/pgloader

### https://stackoverflow.com/questions/109325/postgresql-describe-table?rq=1
    psql -c '\d+ tablename'

### https://stackoverflow.com/questions/512451/how-can-i-add-a-column-to-a-postgresql-database-that-doesnt-allow-nulls

    ALTER TABLE mytable ADD COLUMN mycolumn character varying(50) NOT NULL DEFAULT 'foo';
    ... some work (set real values as you want)...
    ALTER TABLE mytable ALTER COLUMN mycolumn DROP DEFAULT;

    ALTER TABLE statis_xequip ALTER game1 SET DEFAULT 0

### https://stackoverflow.com/questions/15644152/list-tables-in-a-postgresql-schema
Schema

    \d+
    \dt *.*
    \dt public.*
    \dt (public|s).(s|t)

    psql mydb -c "\d information_schema.tables"
    psql mydb -c "SELECT table_catalog,table_schema,table_name,table_type FROM information_schema.tables"

    psql mydb -c "SELECT nspname from pg_catalog.pg_namespace"
    psql mydb -c "SELECT * from pg_catalog.pg_namespace"

### https://wiki.archlinux.org/index.php/PostgreSQL#Upgrading_PostgreSQL

### trigger

http://big-elephants.com/2015-10/writing-postgres-extensions-part-i/
https://oschina.net/translate/postgresql-triggers-golang # https://github.com/rapidloop/ptgo
https://stackoverflow.com/questions/20827761/could-not-access-file-libdir-plpgsql-no-such-file-or-directory
https://www.postgresql.org/docs/9.6/static/xfunc-c.html
https://www.postgresql.org/docs/9.6/static/triggers.html
https://www.postgresql.org/docs/9.6/static/trigger-example.html

    pg_config --libdir
    pg_config --pkglibdir
    cp -t `pg_config --pkglibdir` ptgo.so 

    psql <<< "\d pg_trigger"
    psql <<< "SELECT tgrelid,tgname,tgtype,tgargs FROM pg_trigger"
    psql <<< "SELECT * FROM pg_language"

    DROP TRIGGER IF EXISTS mytrigger ON urls
    CREATE FUNCTION mytrigger() RETURNS TRIGGER AS 'ptgo' LANGUAGE C;
    CREATE TRIGGER trig_1 AFTER INSERT OR UPDATE ON urls FOR EACH ROW EXECUTE PROCEDURE mytrigger();

    ...
    INSERT INTO urls VALUES ('http://example.com/');
    UPDATE urls SET url='http://www.test.com/';

### compare timestamp/UNIX_Time
https://dba.stackexchange.com/questions/2796/how-do-i-get-the-current-unix-timestamp-from-postgresql

    SELECT * from k9ss where EXTRACT(epoch from tims)>1512316800 order by id limit 3
    SELECT * FROM mytab WHERE xtime BETWEEN '2012-04-01 23:55:00'::timestamp AND now()::timestamp

### SELECT ::date

    psql equipment_usages -c "SELECT * from statis_xequip where game1t::date='2018-05-10'"

### https://stackoverflow.com/questions/27037990/connecting-to-postgres-via-database-url-and-unix-socket-in-rails

You must omit the host to use unix socket, like so:

    postgres://username@/dbname

or simply

    postgres:///dbname

This works with psql > 9.2.

///.env
DATABASE_URL=postgres:///dbname


### SELECT & UNION & SUBQUERY

    psql `date +mydb%y%m%d` -c "select id,name from entitys where id IN ((select id from entitys where id<=8 limit 3) UNION (select id from entitys where id>8 order by id limit 3) ) order by id"


psql xapp <<< "SELECT table_schema,table_name,table_type FROM information_schema.tables"

psql xapp <<< "\d+ information_schema.tables"

### https://wiki.archlinux.org/title/PostgreSQL#Upgrading_PostgreSQL
    which pg_upgrade

===   #!/bin/sudo /bin/bash

    pacman -Qs postgresql-old-upgrade
    #
    sudo mv /var/lib/postgres/data /var/lib/postgres/data_old
    sudo mkdir /var/lib/postgres/data /var/lib/postgres/tmp
    sudo chown postgres:postgres /var/lib/postgres /var/lib/postgres/data /var/lib/postgres/tmp
    #
    sudo -iu postgres -- initdb --locale=en_US.UTF-8 -E UTF8 --auth-local trust -D /var/lib/postgres/data
    #
    # #shared_preload_libraries = 'timescaledb'
    sudo vim /var/lib/postgres/data/postgresql.conf  /var/lib/postgres/data_old/postgresql.conf
    #
    sudo systemctl stop postgresql.service
    sudo -iu postgres -- pg_upgrade -b /opt/pgsql-14/bin -B /usr/bin -d /var/lib/postgres/data_old -D /var/lib/postgres/    sudo systemctl start postgresql.service
    #
    psql -l


