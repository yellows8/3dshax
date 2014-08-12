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
