# KDE Purpose Example Plugin

## Introduction

This is a example plugin for purpose. It does compile a fully working .so that can be loaded by all applications which
use purpose to provide actions or services for certain mimetypes.

The plugin itself is probably useless to you, as it will just upload an image to my private image hosting service, but it should give you a great starting point to implement your own plugin in purpose and use it in apps like spectacle.

## Compile Instructions

1. git clone
2. cd cloned_repo
3. mkdir build && cd build
4. cmake ..
5. make

You'll now find a MydayyyImageUpload.so which can be loaded by KDE-Purpose

## Load the Plugin

You need to copy the plugin to /usr/lib/qt/plugins/purpose ( probably depends on your distribution. I am using [Archlinux](https://www.archlinux.org/) btw )