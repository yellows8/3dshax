/*typedef struct {
	u8 ID_size;
	u8 palettetype;//0=none, 1=palette used
	u8 colortype;//2=rgb, etc

	u16 palette_firstent;
	u16 palette_totalcolors;
	u8 palette_colorbitsize;

	u16 xorigin;
	u16 yorigin;
	u16 width;
	u16 height;
	u8 bpp;
	u8 descriptor;//bit4 set=horizontal flip, bit5 set=vertical flip
} PACKED tga_header;*/

/*u32 load_tga(u16 *lowpath, u32 lowpathsize, u8 *out_gfx, u32 outcolor)
{
	u32 x, y, pos;
	u32 *fileobj = NULL;
	u32 *tgabuf = (u32*)0x20200000;
	tga_header *tgahdr = (tga_header*)tgabuf;
	u8 *tmp_gfx = (u8*)((u32)tgabuf + sizeof(tga_header));
	u8 *in_pixel, *out_pixel;
	u16 *out_pix16;
	u32 outalpha=0, inalpha=0, alphaval=0;
	u32 in_pixsz, out_pixsz;
	u32 out_imagesz;
	u8 in_pix[4];

	if(outcolor & 8)
	{
		outcolor &= ~8;
		outalpha = 1;
		alphaval = 0xff;
	}

	if(openfile(sdarchive_obj, 4, lowpath, lowpathsize, 1, &fileobj)!=0)return 4;
	input_filesize = getfilesize(fileobj);
	fileread(fileobj, tgabuf, input_filesize, 0);

	//if(tgahdr->palettetype!=0 || tgahdr->colortype!=2)return 1;
	//if(outcolor==0 && (tgahdr->height>240 || (tgahdr->width!=320 && tgahdr->width!=400)))return 3;

	in_pixsz = tgahdr->bpp / 8;
	if(in_pixsz==32)inalpha = 1;

	out_pixsz = 2;
	if(outcolor==1)out_pixsz = 3+outalpha;

	out_imagesz = out_pixsz * tgahdr->width*tgahdr->height;

	if(tgahdr->palettetype==0)
	{
		if(outcolor==0)
		{
			for(y=0; y<tgahdr->height; y++)
			{
				for(x=0; x<tgahdr->width; x++)
				{
					in_pixel = &tmp_gfx[((y*tgahdr->width) + x) * in_pixsz];
					out_pixel = &out_gfx[((x * tgahdr->height) + (tgahdr->height-1 - y)) * 3];

					out_pixel[0] = in_pixel[0];//in B -> out B
					out_pixel[1] = in_pixel[1];//in G -> out G
					out_pixel[2] = in_pixel[2];//in R -> out R
				}
			}
		}
		else
		{
			for(x=0; x<tgahdr->width; x++)
			{
				for(y=0; y<tgahdr->height; y++)
				{
					in_pixel = &tmp_gfx[((y*tgahdr->width) + x) * in_pixsz];
					out_pixel = &out_gfx[((y*tgahdr->width) + x) * out_pixsz];
					out_pix16 = (u16*)out_pixel;

					in_pix[0] = in_pixel[2+inalpha];
					in_pix[1] = in_pixel[1+inalpha];
					in_pix[2] = in_pixel[0+inalpha];
					//if(outalpha)in_pix[3] = alphaval;
					//memcpy(in_pix, in_pixel, 3);

					if(in_pixsz==4)alphaval = in_pixel[3];

					if(outcolor==1)
					{
						if(outalpha==0)
						{
							out_pixel[0] = in_pix[2];
							out_pixel[1] = in_pix[1];
							out_pixel[2] = in_pix[0];
						}
						else
						{
							out_pixel[0] = in_pix[0];
							out_pixel[1] = in_pix[1];
							out_pixel[2] = in_pix[2];
						}
						if(outalpha)out_pixel[3] = alphaval;
					}
					if(outcolor==2)*out_pix16 = ((in_pix[2]>>3) & 0x1f) | (((in_pix[1]>>2) & 0x3f)<<5) | (((in_pix[0]>>3) & 0x1f)<<11);//BGR565
					if(outcolor==3)*out_pix16 = ((in_pix[0]>>4) & 0xf) | (((in_pix[1]>>4) & 0xf)<<4) | (((in_pix[2]>>4) & 0xf)<<8) | (((alphaval>>4) & 0xf)<<12);//RGBA4444
					if(outcolor==4)
					{
						if(alphaval)alphaval = 1;
						*out_pix16 = ((in_pix[0]>>3) & 0x1f) | (((in_pix[1]>>3) & 0x1f)<<5) | (((in_pix[2]>>3) & 0x1f)<<10) | (alphaval<<15);//RGBA5551
					}
				}
			}
		}
	}
	else
	{
		in_pixsz = tgahdr->palette_colorbitsize / 8;

		memset(out_gfx, 0, 0x100*in_pixsz);
		memcpy(out_gfx, &tmp_gfx[tgahdr->palette_firstent], tgahdr->palette_totalcolors * in_pixsz);
		memcpy(&out_gfx[0x100*in_pixsz], &tmp_gfx[tgahdr->palette_firstent + tgahdr->palette_totalcolors * in_pixsz], tgahdr->width*tgahdr->height);
	}

	return 0;
}*/

/*void a9oldgfx_main()
{*/
	//u32 *framebuf;

	/*if(framebuf_addr)
	{
		framebuf = framebuf_addr;
		memset(framebuf_addr, 0xffffffff, 0x46500);
		memset(&framebuf_addr[(0x46500+0x10)>>2], 0xffffffff, 0x46500);
		load_tga(maingfx0tga_filepath, 0x1c, (u8*)framebuf, 0);//Main screen frambuffers for left 3D image
		memcpy(&framebuf[(0x46500+0x10)>>2], (u8*)framebuf, 0x46500);
		framebuf = &framebuf[((0x46500*2) + (0x10*2))>>2];

		load_tga(subgfxtga_filepath, 0x18, (u8*)framebuf, 0);//Sub screen framebuffers
		memcpy(&framebuf[(0x38400+0x10)>>2], (u8*)framebuf, 0x38400);
		framebuf = &framebuf[((0x38400*2) + (0x10*2))>>2];

		load_tga(maingfx1tga_filepath, 0x1c, (u8*)framebuf, 0);//Main screen frambuffers for right 3D image
		memcpy(&framebuf[(0x46500+0x10)>>2], framebuf, 0x46500);
		framebuf = &framebuf[((0x46500*2) + (0x10*2))>>2];
	}*/

	//loadfile(0x20703000, 0x20000, input_filepath, 0x24);

	/*while(1)
	{
		if((*((vu16*)0x10146000) & 0x10) == 0)
		{*/
			/*for(pos=0; pos<3; pos+=3)
			{
				((u16*)0x18464ef8)[0+pos] = (((u16*)0x18464ef8)[0+pos] + 1) & 0xef;//0x18464ef8
				((u16*)0x18464ef8)[1+pos] = (((u16*)0x18464ef8)[1+pos] + 1) & 0xef;
				((u16*)0x18464ef8)[2+pos] = (((u16*)0x18464ef8)[2+pos] + 1) & 0xef;
			}*/
			//memset((u32*)0x18464ef8, 0x3f800000, 0x8);//0x3f800000
			//memset((u32*)0x182447C0+0x100000, 0x3f800000, 0x300000-0x100000);
			//memset((u32*)0x185447C0, 0x3f800000, 0x18600000-0x185447C0);
		//}
	//}
	//memset((u32*)0x1846a66c, 0xffffffff, 0x400);//0x3f800000
	//memset((u32*)0x1845dd00, 0x0, 0x18464b18-0x1845dd00);
	//memset((u32*)0x1846d400, 0x3f800000, 0x1000);
	//memset((u32*)0x18464b18, 0, 0x1846a66c-0x18464b18);
	//memset((u32*)0x185447C0, 0, 0x600000 - 0x5447C0);
	//memset((u32*)0x18061d58, 0x3f800000, 0x100);
	//memcpy((u32*)0x18061d58, vertices, 24*4);
	//for(pos=0; pos<36; pos++)vert_elements[pos] += 2;
	//memcpy((u16*)0x18469bc0, vert_elements, 36*2);

	/*for (pos = 1; pos < 6; pos++)memcpy(&cube_texcoords[pos*4*2], &cube_texcoords[0], 2*4*sizeof(float));
	memcpy((float*)0x18061d58, cube_vertices, 6*4*3*sizeof(float));
	memcpy((float*)0x18068a90, cube_texcoords, 2*4*6*sizeof(float));
	memcpy((u16*)0x18469bc0, cube_elements, 36*2);*/

	//dump_fcramvram();

	//pos = load_tga(texturetga_filepath, 0x1a, (u8*)0x20703000, 1 | 8);//(0x1846F480)0x208AFF80
	//loadfile((u8*)(0x1846F480-0x44), 0x844, texturektx_filepath, 0x1a);
	//loadfile((u8*)(0x1846F480-0x10), 0x810, texturepkm_filepath, 0x1a);
	//memset((u8*)0x1846F480, 0xffffffff, 128*32*4);
	//dumpmem(&pos, 4);

	//while(*((vu16*)0x10146000) & 0x400);
	//dumpmem((u32*)0x20313890, 0x46500);

	/*loadfile(((u32*)(0x20313890)), 0x46500, maingfx0bin_filepath, 0x1c);
	loadfile(((u32*)(0x20359da0)), 0x46500, maingfx0bin_filepath, 0x1c);
	loadfile(((u32*)(0x20410ad0)), 0x46500, maingfx1bin_filepath, 0x1c);
	loadfile(((u32*)(0x20456fe0)), 0x46500, maingfx1bin_filepath, 0x1c);

	loadfile(((u32*)(0x203A02b0)), 0x38400, subgfxbin_filepath, 0x18);
	loadfile(((u32*)(0x203A02b0+0x38400+0x10)), 0x38400, subgfx_filepath, 0x18);*/

	//dumpmem(0x20703000+0x20000, 0x20000);
//}

