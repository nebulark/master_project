extern crate mini_gl_fb;
extern crate nalgebra;
extern crate ncollide3d;

mod gof;

use mini_gl_fb::{BufferFormat, Config};
use nalgebra::{Unit, Vector3};

#[repr(C)]
struct Color {
    red: u8,
    green: u8,
    blue: u8,
}

impl Color {
    fn new(red: u8, green: u8, blue: u8) -> Color {
        Color { red, green, blue }
    }
}

struct Ray {
    origin: Vector3<f32>,
    dir: Unit<Vector3<f32>>,
}

const ROWS: i32 = 800;
const COLLS: i32 = 600;
const PIXELS: i32 = ROWS * COLLS;
const ROWS_F: f32 = ROWS as f32;
const COLLS_F: f32 = COLLS as f32;
const ROWS_INV_F: f32 = 1.0 / ROWS_F;
const COLLS_INV_F: f32 = 1.0 / COLLS_F;

fn main() {
    let mut fb = mini_gl_fb::get_fancy(Config {
        window_title: "test",
        window_size: (800.0, 600.0),
        buffer_size: (2, 2),
        ..Default::default()
    });

    fb.change_buffer_format::<u8>(BufferFormat::RGB);
    let buffer = vec![
        Color::new(255, 0, 0),
        Color::new(0, 255, 0),
        Color::new(0, 0, 0),
        Color::new(0, 0, 255),
    ];
    let field_of_view_deg: f32 = 90.;
    let field_of_view_rad = f32::to_radians(field_of_view_deg);
    let field_of_view_scaling = f32::tan(field_of_view_rad / 2.0f32);

    fb.update_buffer(&buffer);
    fb.persist();
}
