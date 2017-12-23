
### http://stackoverflow.com/questions/11269575/how-to-hide-output-of-subprocess-in-python-2-7/11270665#11270665

    p = Popen(['espeak', '-b', '1'], stdin=PIPE, stdout=DEVNULL, stderr=STDOUT)
    p.communicate(text.encode('utf-8'))
    assert p.returncode == 0 # use appropriate for your program error handling here

### http://stackoverflow.com/questions/89228/calling-an-external-command-in-python?rq=1

    p = Popen(['awk', 'length($0)>1' ], stdout=PIPE)

### http://stackoverflow.com/questions/13332268/python-subprocess-command-with-pipe?rq=1

    ps = subprocess.Popen(('ps', '-A'), stdout=subprocess.PIPE)
    output = subprocess.check_output(('grep', 'process_name'), stdin=ps.stdout)
    ps.wait()

### http://stackoverflow.com/questions/4760215/running-shell-command-from-python-and-capturing-the-output?rq=1

    def run_command(command):
        if type(command) == str:
            command = command.split()
        p = subprocess.Popen(command,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        return iter(p.stdout.readline, b'')
    command = 'mysqladmin create test -uroot -pmysqladmin12'
    for line in run_command(command):
        print(line)

