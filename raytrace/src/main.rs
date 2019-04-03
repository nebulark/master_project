extern crate mini_gl_fb;
extern crate nalgebra;
extern crate ncollide3d;

mod gof;

use mini_gl_fb::{BufferFormat, Config};
use nalgebra::{Isometry3, Point2, Point3, Unit, Vector3};
use ncollide3d::query::{Ray, RayCast};
use ncollide3d::shape::Cuboid;

#[repr(C)]
#[derive(Default, Copy, Clone)]
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

struct Camera {
    up: Unit<Vector3<f32>>,
    right: Unit<Vector3<f32>>,
    forward: Unit<Vector3<f32>>,
}

impl Camera {
    fn new_with_forward_dir(forward: Unit<Vector3<f32>>) -> Self {
        let world_up: Unit<Vector3<f32>> = Vector3::y_axis();
        let right: Unit<Vector3<f32>> = Unit::new_normalize(forward.cross(&world_up));
        let up: Unit<Vector3<f32>> = Unit::new_normalize(right.cross(&forward));
        Camera { up, right, forward }
    }

    /// normalised_screen_pixel: should be in range from -1 to +1. One of the two components may be
    /// greater to adjust for Aspect ratio
    fn calc_ray_dir(
        &self,
        normalised_screen_pixel: Point2<f32>,
        field_of_view_scaling: f32,
    ) -> Unit<Vector3<f32>> {
        // we treat camera origin as 0, we are only interested in direction so it does not matter
        // if the two points whicht define that direction are translated by the same amount

        let projection_plane_origin = Point3::from(*self.forward.as_ref()); // + Point3::origin(); // Point3::from(self.forward);
        let point_on_projection_plane = projection_plane_origin
            + normalised_screen_pixel.x * self.right.as_ref() * field_of_view_scaling
            + normalised_screen_pixel.y * self.up.as_ref() * field_of_view_scaling;

        // we can just use the normalized point as the origin is 0.
        Unit::new_normalize(point_on_projection_plane.coords)
    }
}

const ROWS: u32 = 800;
const COLLS: u32 = 600;
const PIXELS: u32 = ROWS * COLLS;
const ROWS_F: f32 = ROWS as f32;
const COLLS_F: f32 = COLLS as f32;
const ROWS_INV_F: f32 = 1.0 / ROWS_F;
const COLLS_INV_F: f32 = 1.0 / COLLS_F;

fn main() {
    let mut fb = mini_gl_fb::get_fancy(Config {
        window_title: "test",
        window_size: (800.0, 600.0),
        buffer_size: (COLLS, ROWS),
        ..Default::default()
    });

    fb.change_buffer_format::<u8>(BufferFormat::RGB);
    let buffer = vec![
        Color::new(255, 0, 0),
        Color::new(0, 255, 0),
        Color::new(0, 0, 0),
        Color::new(0, 0, 255),
    ];
    let mut vec: Vec<Color> = vec![Default::default(); PIXELS as usize];
    let field_of_view_deg: f32 = 90.;
    let field_of_view_rad = f32::to_radians(field_of_view_deg);
    let field_of_view_scaling = f32::tan(field_of_view_rad / 2.0f32);
    let aspect_ratio_scaling = COLLS_F / ROWS_F;
    let cam = Camera::new_with_forward_dir(Vector3::z_axis());
    let cuoid = Cuboid::new(Vector3::new(1.0, 2.0, 1.0));
    {
        let mut x = 0;
        let mut y = 0;
        for elem in &mut vec {
            let norm_x = x as f32 * COLLS_INV_F;
            let norm_y = y as f32 * ROWS_INV_F;

            let screen_x = (norm_x * 2.0) - 1.0;
            let screen_y = (norm_y * 2.0) - 1.0;
            let normalised_screen_pixel = Point2::new(screen_x * aspect_ratio_scaling, screen_y);
            let ray_dir = cam.calc_ray_dir(normalised_screen_pixel, field_of_view_scaling);

            let ray = Ray::new(Point3::new(3.0, 3.0, -10.0), ray_dir.into_inner());

            let raycast_result = cuoid.toi_and_normal_with_ray(&Isometry3::identity(), &ray, true);

            let normal_as_color = match raycast_result {
                Some(res) => res.normal * 128.0 + Vector3::repeat(127.0f32),
                None => Vector3::repeat(0.0f32),
            };

            *elem = Color::new(
                normal_as_color.x as u8,
                normal_as_color.y as u8,
                normal_as_color.z as u8,
            );
            // Do Raytrace
            x += 1;
            if x == COLLS {
                x = 0;
                y += 1;
            }
        }
        assert!(y == ROWS && x == 0);
    }

    fb.update_buffer(&vec);
    fb.persist();
}
