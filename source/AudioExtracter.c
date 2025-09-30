// #include <stdio.h>
// #include <stdlib.h>
// #include <stdint.h>
// #include "smacker.h"

// static void write_wav_header(FILE *f,
//                              unsigned long sample_rate,
//                              unsigned char channels,
//                              unsigned char bits,
//                              uint32_t data_size)
// {
//     uint32_t byte_rate = sample_rate * channels * bits/8;
//     uint16_t block_align = channels * bits/8;

//     fwrite("RIFF",1,4,f);
//     uint32_t chunk_size = 36 + data_size;
//     fwrite(&chunk_size,4,1,f);
//     fwrite("WAVE",1,4,f);

//     fwrite("fmt ",1,4,f);
//     uint32_t subchunk1_size = 16;
//     fwrite(&subchunk1_size,4,1,f);
//     uint16_t audio_format = 1; // PCM
//     fwrite(&audio_format,2,1,f);
//     fwrite(&channels,2,1,f);
//     fwrite(&sample_rate,4,1,f);
//     fwrite(&byte_rate,4,1,f);
//     fwrite(&block_align,2,1,f);
//     fwrite(&bits,2,1,f);

//     fwrite("data",1,4,f);
//     fwrite(&data_size,4,1,f);
// }

// int main(int argc, char **argv){
//     if(argc < 3){
//         fprintf(stderr,"Usage: %s input.smk output.wav\n",argv[0]);
//         return 1;
//     }

//     smk movie = smk_open_file(argv[1], SMK_MODE_DISK);
//     if(!movie){
//         fprintf(stderr,"Cannot open %s\n",argv[1]);
//         return 1;
//     }

//     unsigned char track_mask=0;
//     unsigned char channels[7]={0};
//     unsigned char bitdepth[7]={0};
//     unsigned long rate[7]={0};
//     smk_info_audio(movie,&track_mask,channels,bitdepth,rate);

//     // prendi il primo canale disponibile
//     int track=-1;
//     for(int i=0;i<7;i++){
//         if(track_mask & (1<<i)){ track=i; break; }
//     }
//     if(track<0){
//         fprintf(stderr,"No audio track found!\n");
//         smk_close(movie);
//         return 1;
//     }

//     printf("Using audio track %d: %lu Hz, %u ch, %u bits\n",
//             track, rate[track], channels[track], bitdepth[track]);

//     smk_enable_audio(movie, track, 1);
//     smk_first(movie);

//     FILE *out=fopen(argv[2],"wb");
//     if(!out){ perror("fopen"); smk_close(movie); return 1; }

//     // header placeholder
//     write_wav_header(out, rate[track], channels[track], bitdepth[track], 0);
//     long data_start=ftell(out);
//     uint32_t total_bytes=0;

//     int done=0;
//     while(!done){
//         const unsigned char *chunk=smk_get_audio(movie,track);
//         unsigned long size=smk_get_audio_size(movie,track);
//         if(chunk && size>0){
//             fwrite(chunk,1,size,out);
//             total_bytes+=size;
//         }
//         char r=smk_next(movie);
//         if(r==SMK_DONE || r==SMK_ERROR) done=1;
//     }

//     // patch header
//     fseek(out,0,SEEK_SET);
//     write_wav_header(out, rate[track], channels[track], bitdepth[track], total_bytes);
//     fclose(out);

//     smk_close(movie);
//     return 0;
// }
