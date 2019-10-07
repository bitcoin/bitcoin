// Common functions that are unfortunately missing on illumos and
// Solaris, but often needed by other crates.

use unix::solarish::*;

pub unsafe fn cfmakeraw(termios: *mut ::termios) {
    let mut t = *termios as ::termios;
    t.c_iflag &= !(IMAXBEL
        | IGNBRK
        | BRKINT
        | PARMRK
        | ISTRIP
        | INLCR
        | IGNCR
        | ICRNL
        | IXON);
    t.c_oflag &= !OPOST;
    t.c_lflag &= !(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t.c_cflag &= !(CSIZE | PARENB);
    t.c_cflag |= CS8;
}

pub unsafe fn cfsetspeed(
    termios: *mut ::termios,
    speed: ::speed_t,
) -> ::c_int {
    // Neither of these functions on illumos or Solaris actually ever
    // return an error
    ::cfsetispeed(termios, speed);
    ::cfsetospeed(termios, speed);
    0
}
