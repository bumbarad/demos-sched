# builds demos-sched with different versions of gcc and meson to test compatibility
{
  pkgsUnstable ? import (fetchTarball "https://github.com/NixOS/nixpkgs-channels/archive/nixos-unstable.tar.gz") { }
}:
let
  pkgsMeson-0-49-2 = import (builtins.fetchTarball {
    url = "https://github.com/nixos/nixpkgs/archive/4599f2bb9a5a6b1482e72521ead95cb24e0aa819.tar.gz";
    sha256 = "04xr4xzcj64d5mf4jxzn5fsbz74rmf90qddp3jcdwj4skyik946d";
  }) { };
  demosSchedWithCC = { pkgs, cc, overrideMesonCC ? false, withSubmodules ? false }:
    let
      myStdenv = with pkgs; (overrideCC stdenv (builtins.getAttr cc pkgs));
    in with pkgs;
    callPackage ../demos-sched.nix ({
      stdenv = myStdenv;
      meson = meson.overrideAttrs (attrs: { depsHostHostPropagated = [ ]; });
      libyamlcpp = libyamlcpp.override { stdenv = myStdenv; };
      inherit withSubmodules;
    } // pkgs.lib.optionalAttrs overrideMesonCC {
      extraAttrs = {
        # Meson in old nixpkgs hardcodes CC and CXX and does not obey stdenv - override it here
        preConfigure = ''
          meson() {
            unset CC CXX
            command meson "$@"
          }
        '';
      };
    });
in {
  unstable = with pkgsUnstable; callPackage ../demos-sched.nix { };
  unstable-subprojects = with pkgsUnstable; callPackage ../demos-sched.nix { withSubmodules = true; };
  meson-0-49-2 = demosSchedWithCC { pkgs = pkgsMeson-0-49-2; cc = "gcc8"; overrideMesonCC = true; withSubmodules = true; };
  gcc8 = demosSchedWithCC { pkgs = pkgsUnstable; cc= "gcc8"; withSubmodules = true; };
  gcc9 = demosSchedWithCC { pkgs = pkgsUnstable; cc= "gcc9"; withSubmodules = true; };
}
