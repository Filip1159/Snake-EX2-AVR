package com.nokia.mid.ui;
import javax.microedition.lcdui.Image;
public interface DirectGraphics { void drawPixels(short[] p,boolean t,int o,int s,int x,int y,int w,int h,int m,int f); void drawPixels(int[] p,boolean t,int o,int s,int x,int y,int w,int h,int m,int f); void getPixels(int[] p,int o,int s,int x,int y,int w,int h,int f); void drawImage(Image i,int x,int y,int a,int m); }
