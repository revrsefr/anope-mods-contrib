# Welcome to the anope-mods-contrib wiki!

Wikis provide a place in your repository to lay out the roadmap of your project, show the current status, and document software better, together.

Note: this repository currently stores wiki-style pages in the main repo under the `wiki/` folder.
GitHub’s built-in Wiki (the `/wiki` URL) is a separate git repository (`<repo>.wiki.git`) and needs to be enabled in the repo settings to use it.

## Quick start

1. Copy the module `.cpp` into your Anope source tree (commonly `modules/third/`).
2. Rebuild and install modules.
3. Add the required `module { ... }` block (and any `command { ... }` blocks).
4. Reload config (`/msg OperServ RELOAD`) or restart services.

## Pages

- [Modules](Modules.md)
