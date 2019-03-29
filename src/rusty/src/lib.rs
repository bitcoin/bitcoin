#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}

#[no_mangle]
pub static RUST_CONSTANT: i32 = 43;

#[no_mangle]
pub extern "C" fn hello_world() {
    println!("Hello World!");
}
