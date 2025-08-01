  // This is the command sequence that rotates the ILI9488 driver coordinate frame

  writecommand(TFT_MADCTL);
  rotation = m % 4;
  switch (rotation) {
   case 0: // Portrait
     writedata(TFT_MAD_MX | TFT_MAD_BGR);
      _width  = TFT_WIDTH;
      _height = TFT_HEIGHT;
     break;
   case 1: // Landscape (Portrait + 90)
     writedata(TFT_MAD_MV | TFT_MAD_BGR);
      _width  = TFT_HEIGHT;
      _height = TFT_WIDTH;
     break;
   case 2: // Inverter portrait
     writedata(TFT_MAD_MY | TFT_MAD_BGR);
      _width  = TFT_WIDTH;
      _height = TFT_HEIGHT;
     break;
   case 3: // Inverted landscape
     writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_BGR);
      _width  = TFT_HEIGHT;
      _height = TFT_WIDTH;
     break;
  }
   
