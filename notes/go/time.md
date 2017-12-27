
### https://stackoverflow.com/questions/20234104/how-to-format-current-time-using-a-yyyymmddhhmmss-format
### http://www.golangprograms.com/golang/date-time/

    lt := time.Now().Local()
    fmt.Println("YYYY-MM-DD hh:mm:ss:", lt.Format("2006-01-02 15:04:05"))

