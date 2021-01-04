
    rust-script --dep time --expr "time::OffsetDateTime::now_utc().format(time::Format::Rfc3339).to_string()"

    cat now.rs | rust-script --count --loop '|l,n|{ println!("{:>6}: {}",n,l.trim_right()) }'

