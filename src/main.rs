extern crate sdl2;

fn main() {
    let sdl = sdl2::init().unwrap();
    
    let video_subsystem = sdl.video().unwrap();
    let window = video_subsystem
        .window("Game", 900, 700)
        .resizable()
        .build()
        .unwrap();
    loop {

    }
}
