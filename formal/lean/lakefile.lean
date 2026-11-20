import Lake
open Lake DSL

package blade_formal

@[default_target]
lean_lib BladeFormal where
  globs := #[.submodules `BladeFormal]
