use std::env;
use std::process::{Command, Stdio};
use std::sync::mpsc;
use std::thread;
use std::time::Duration;
use std::io::{BufRead, BufReader, Read};

fn main() {
    assert_eq!(env::args().len(), 2);

    let status = Command::new("rumprun-bake")
        .arg("hw_virtio")
        .arg("/tmp/libc-test.img")
        .arg(env::args().nth(1).unwrap())
        .status()
        .expect("failed to run rumprun-bake");
    assert!(status.success());

    let mut child = Command::new("qemu-system-x86_64")
        .arg("-nographic")
        .arg("-vga").arg("none")
        .arg("-m").arg("64")
        .arg("-kernel").arg("/tmp/libc-test.img")
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .expect("failed to spawn qemu");

    let mut stdout = child.stdout.take().unwrap();
    let mut stderr = child.stderr.take().unwrap();
    let (tx, rx) = mpsc::channel();
    let tx2 = tx.clone();
    let t1 = thread::spawn(move || find_ok(&mut stdout, tx));
    let t2 = thread::spawn(move || find_ok(&mut stderr, tx2));

    let res = rx.recv_timeout(Duration::new(5, 0));
    child.kill().unwrap();
    t1.join().unwrap();
    t2.join().unwrap();

    if res.is_err() {
        panic!("didn't find success");
    }
}

fn find_ok(input: &mut Read, tx: mpsc::Sender<()>) {
    for line in BufReader::new(input).lines() {
        let line = line.unwrap();
        println!("{}", line);
        if (line.starts_with("PASSED ") && line.contains(" tests")) ||
            line.starts_with("test result: ok"){
            tx.send(()).unwrap();
        }
    }
}
