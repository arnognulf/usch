#!/bin/sh
licensecheck -r *
debuild -i -us -uc -b

