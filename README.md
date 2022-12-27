# NABU-LIB

For HomeBrew HTTPS://NABU.CA RetroNet of the NABU Persoal Computer Preservation Project. This repo is a z88dk C library for the NABU PC (personal computer). It includes efficient C functions for video, audio, hcca, keyboard, and more for the NABU Computer. This is an ongoing project, so follow this repo for additions, changes and bug fixes

This library is optimized to be as efficient as possible, including text mode double buffer, special function registers, inlining, and fastcall where appropriate. This project aims to build a library for homebrew development of the NABU PC with the easiest and fastest toolset. 

To use this library, you can follow the tutorial here: https://nabu.ca/homebrew-c-tutorial

## v2012.12.26.01
Added a new function for keyboard input called readLine(). Also added an example on how to use it (Example - User Input and Print)

## v2012.12.26.00

This update includes http get features and requires the most recent Internet Adapter from https://nabu.ca. Now NABU HomeBrew applications can query data on the internet and the Internet Adapter will cache it. When the NABU application needs the data, it can request it from the Internet Adapter. Up to 256 queries per application can be stored in the IA for each NABU HomeBrew program. This can include additional code, music, images, etc.. Essentially, this gives a NABU a practically unlimited amount of storage space because it can use the cloud for obtaining resources. 
