#[derive(Debug)]
pub enum Error {
    MissingBytes(usize),
    Todo,
}

pub type Result<T> = core::result::Result<T, Error>;

impl From<()> for Error {
    fn from(_: ()) -> Self {
        Error::Todo
    }
}

impl core::fmt::Display for Error {
    fn fmt(&self, _: &mut core::fmt::Formatter<'_>) -> core::result::Result<(), core::fmt::Error> {
        Ok(())
    }
}
