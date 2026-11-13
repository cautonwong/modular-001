# Products

Each product descriptor selects components and a platform. It contains no
business rules. Product code lives in the matching target directory when the
product has a build.

The descriptors are currently planning and validation inputs. `tools/check_components.py`
checks that every listed component exists and that component manifests are
consistent.
