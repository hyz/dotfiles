

    fd --path-separator='/' -e md . src/examples -x rust-script -e 'println!("[{}]");'

    fd --path-separator='/' -e meta . src/examples -x rust-script -e '_=std::fs::remove_file("{}");'
    fd --path-separator='/' -e meta . src/examples | rust-script -l '|p|{std::fs::remove_file(p.trim()).unwrap();}'

    rust-script --dep time --expr "time::OffsetDateTime::now_utc().format(time::Format::Rfc3339).to_string()"

    cat now.rs | rust-script --count --loop '|l,n|{ println!("{:>6}: {}",n,l.trim_right()) }'

