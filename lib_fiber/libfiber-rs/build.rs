extern crate cc;
extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    // Tell cargo to invalidate the built crate whenever the wrapper changes
    println!("cargo:rerun-if-changed=../c/include/fiber/libfiber.h");
    // The bindgen::Builder is the main entry point
    // to bindgen, and lets you build up options for
    // the resulting bindings.
    bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("../c/include/fiber/libfiber.h")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings")
        // Write the bindings to the src/bindings.rs file.
        .write_to_file("src/libfiber.rs")
        .expect("Couldn't write bindings!");

    println!("cargo:rustc-link-search=native=../lib");
    println!("cargo:rustc-link-lib=dylib=libfiber");
}