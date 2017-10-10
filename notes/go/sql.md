
### http://www.calhoun.io/using-postgresql-with-golang/

### https://www.calhoun.io/inserting-records-into-a-postgresql-database-with-gos-database-sql-package/

### https://www.calhoun.io/updating-and-deleting-postgresql-records-using-gos-sql-package/

Updating and deleting PostgreSQL records using Go's sql package


### https://github.com/lib/pq/issues/24

    var id int
    if err := db.QueryRow("INSERT INTO user (name) VALUES ('John') RETURNING id").Scan(&id); err != nil {}

