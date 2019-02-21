extern crate gl;
extern crate sdl2;

use std::ffi::{CStr, CString};

fn main() {
    let sdl = sdl2::init().unwrap();

    let video_subsystem = sdl.video().unwrap();

    let gl_attr = video_subsystem.gl_attr();

    gl_attr.set_context_profile(sdl2::video::GLProfile::Core);
    gl_attr.set_context_version(3, 3);

    let window = video_subsystem
        .window("Game", 900, 700)
        .opengl()
        .resizable()
        .build()
        .unwrap();

    let gl_context = window.gl_create_context().unwrap();

    let gl =
        gl::load_with(|s| video_subsystem.gl_get_proc_address(s) as *const std::os::raw::c_void);

    let mut event_pump = sdl.event_pump().unwrap();

    unsafe {
        gl::Viewport(0, 0, 900, 700);
        gl::ClearColor(0.3, 0.3, 0.5, 1.0);
    }

    'main: loop {
        for event in event_pump.poll_iter() {
            //handle user input
            match event {
                sdl2::event::Event::Quit { .. } => break 'main,
                _ => {}
            }
        }

        unsafe {
            gl::Clear(gl::COLOR_BUFFER_BIT);
        }
        window.gl_swap_window();
    }
}
pub mod render_gl {
    use gl;
    use std;
    use std::ffi::{CStr, CString};

    fn shader_from_source(
        source: &CStr,
        kind: gl::types::GLuint,
    ) -> Result<gl::types::GLuint, String> {
        let id = unsafe { gl::CreateShader(kind) };
        unsafe {
            gl::CompileShader(id);
        }
        let success = unsafe {
            let mut success_tmp: gl::types::GLint = 1;
            gl::GetShaderiv(id, gl::COMPILE_STATUS, &mut success_tmp);
            success_tmp
        };

        if success == 0 {
            let len = unsafe {
                let mut len_tmp: gl::types::GLint = 1;
                gl::GetShaderiv(id, gl::INFO_LOG_LENGTH, &mut len_tmp);
                len_tmp
            };
            let error_string = create_whitspace_cstring_with_length(len as usize);
            unsafe {
                gl::GetShaderInfoLog(
                    id,
                    len,
                    std::ptr::null_mut(),
                    error_string.as_ptr() as *mut gl::types::GLchar,
                );
            }
            return Err(error_string.to_string_lossy().into_owned());
        }

        Ok(id)
    }

    struct Shader {
        id: gl::types::GLuint,
    }

    impl Shader {
        fn from_source(source: &CStr, kind: gl::types::GLenum) -> Result<Shader, String> {
            let id = shader_from_source(source, kind)?;
            Ok(Shader { id })
        }
    }

    impl Drop for Shader {
        fn drop(&mut self) {
            unsafe {
                gl::DeleteShader(self.id);
            }
        }
    }

    fn create_whitspace_cstring_with_length(len: usize) -> CString {
        // allocate buffer of correct size
        let mut buffer: Vec<u8> = Vec::with_capacity(len as usize + 1);
        // fill it with len spaces
        buffer.extend([b' '].iter().cycle().take(len as usize));
        // convert buffer to CString
        let result: CString = unsafe { CString::from_vec_unchecked(buffer) };
        result
    }
}
