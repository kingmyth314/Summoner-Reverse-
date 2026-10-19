/* TEMPORARY: the eight libjpeg 6b functions gr_screenshot uses, as no-ops, until libjpeg 6b is built from the IJG
   source with VC6 (Sum.exe links it statically). A JPEG screenshot writes nothing with these. */
void *jpeg_std_error(void *err) { return err; }
void jpeg_CreateCompress(void *cinfo, int version, unsigned size) {}
void jpeg_stdio_dest(void *cinfo, void *outfile) {}
void jpeg_set_defaults(void *cinfo) {}
void jpeg_start_compress(void *cinfo, int write_all_tables) {}
unsigned jpeg_write_scanlines(void *cinfo, void *scanlines, unsigned num_lines) { return num_lines; }
void jpeg_finish_compress(void *cinfo) {}
void jpeg_destroy_compress(void *cinfo) {}
