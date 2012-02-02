program BinToS;

{$APPTYPE CONSOLE}

uses
  SysUtils, Classes;//, HttpApp;


function UnixPathToDosPath(path: string): string;
begin
   result:= StringReplace(path, '/', '\', [rfReplaceAll]);
end;

function rgb24_to_vdp_color(color: integer): integer;
var
   r, g, b: integer;
begin
   r:= color shr 16;
   g:= color shr 8;
   b:= color shr 0;

   // genesis only define on 3 bits (but shifted to 1 left)
   r:= (r shr 4) and $E;
   g:= (g shr 4) and $E;
   b:= (b shr 4) and $E;

   result:= (r shl 0) or (g shl 4) or (b shl 8);
end;

function delete_last(const s: string; del: integer): string;
var
   len: integer;
begin
   len := Length(s);
   result := Copy(s, 1, len - del);
end;



var
   bitmap: boolean;
   ii, jj: integer;
   data: cardinal;
   w, h: integer;
   len: integer;
   offset: integer;
   align: integer;
   sizealign: integer;
   formatint: integer;
   nullfill: integer;
   format: string;
   formatasm: string;
   ss: string;
   shortname: string;
   FileName: string;
   FileNameOut: string;
   FileInput: TFileStream;
   FileOutput: TStringList;
   MemInput: TMemoryStream;
   MemTmp: TMemoryStream;
begin
   // default
   bitmap := false;
   FileName := '';
   FileNameOut := '';
   format := 'u8 ';
   formatasm := 'b';
   formatint := 1;
   align := 4;
   sizealign := 1;
   nullfill := 0;

   // parse parmeters
   ii := 1;
   while (ii <= ParamCount) do
   begin
      if (ParamStr(ii) = '-u8') then begin
         format := 'u8 ';
         formatasm := 'b';
         formatint := 1;
      end else if (ParamStr(ii) = '-s8') then begin
         format := 's8 ';
         formatasm := 'b';
         formatint := 1;
      end else if (ParamStr(ii) = '-u16') then begin
         format := 'u16 ';
         formatasm := 'w';
         formatint := 2;
      end else if (ParamStr(ii) = '-s16') then begin
         format := 's16 ';
         formatasm := 'w';
         formatint := 2;
      end else if (ParamStr(ii) = '-u32') then begin
         format := 'u32 ';
         formatasm := 'l';
         formatint := 4;
      end else if (ParamStr(ii) = '-s32') then begin
         format := 's32 ';
         formatasm := 'l';
         formatint := 4;
      end else if (ParamStr(ii) = '-bmp') then begin
         bitmap := true;
         format := 'u16 ';
         formatasm := 'w';
         formatint := 2;
      end else if (ParamStr(ii) = '-align') then begin
         Inc(ii);
         align := StrToIntDef(ParamStr(ii), 4);
      end else if (ParamStr(ii) = '-sizealign') then begin
         Inc(ii);
         sizealign := StrToIntDef(ParamStr(ii), 1);
      end else if (ParamStr(ii) = '-nullfill') then begin
         Inc(ii);
         nullfill := StrToIntDef(ParamStr(ii), 0);
      end else if (FileName = '') then FileName:= UnixPathToDosPath(ParamStr(ii))
      else if (FileNameOut = '') then FileNameOut:= UnixPathToDosPath(ParamStr(ii));

      Inc(ii);
   end;

   if (FileNameOut = '') then FileNameOut := FileName;

   if (UpperCase(ExtractFileExt(FileName)) = '.BMP') then begin
      bitmap := true;
      format := 'u16 ';
      formatasm := 'w';
      formatint := 2;
      align := 16;
   end;

   if (not FileExists(FileName)) then MemInput := nil
   else begin
      MemInput := TMemoryStream.Create;
      FileInput := TFileStream.Create(FileName, fmOpenRead or fmShareDenyWrite);
      // we use a memory copy of the file
      try
         MemInput.CopyFrom(FileInput, 0);
         MemInput.Seek(0, soFromBeginning);
      finally
         FileInput.Free;
      end;
   end;

   FileOutput := TStringList.Create;
   try
      offset := 0;
      if (not Assigned(MemInput)) then len := 1
      else len := MemInput.Size;

      if (bitmap) then Dec(len, 118 - ((2 * 2) + (2 * 16)));

      // adjust size for word/long boundary
      len := (len + (formatint - 1)) and (not (formatint - 1));
      // size alignement needed ?
      if ((len and Pred(sizealign)) <> 0) then
         len := (len and (not Pred(sizealign))) + sizealign;

      shortname := ChangeFileExt(ExtractFileName(FileName), '');

      FileOutput.Add('.text');
      FileOutput.Add('');
      FileOutput.Add('    align ' + IntToStr(align));
      FileOutput.Add('');
      FileOutput.Add('    .globl  ' + shortname);
      FileOutput.Add(shortname + ':');

      if (Assigned(MemInput)) then
      begin
         if (bitmap) then
         begin
            MemInput.Seek(18, soFromBeginning);

            // get width
            MemInput.Read(w, 4);
            ss := '0x' + IntToHex(w, 4) + ', ';
            // get height
            MemInput.Read(h, 4);
            ss := ss + '0x' + IntToHex(h, 4);

            FileOutput.Add('    dc.' + formatasm + '  ' + ss);

            MemTmp := TMemoryStream.Create;
            try
               // reverse lines
               for ii := (h - 1) downto 0 do
               begin
                  MemInput.Seek(118 + ((ii * w) div 2), soFromBeginning);
                  for jj := 0 to ((w div 8) - 1) do
                  begin
                     MemInput.Read(data, 4);
                     // reverse high and low byte
                     data := ((data shr 24) and $000000FF) or
                             ((data shr 08) and $0000FF00) or
                             ((data shl 08) and $00FF0000) or
                             ((data shl 24) and $FF000000);
                     data := ((data shr 16) and $0000FFFF) or
                             ((data shl 16) and $FFFF0000);
                     MemTmp.Write(data, 4);
                  end;
               end;

               // erase old lines
               MemInput.Seek(118, soFromBeginning);
               for ii := 0 to (h - 1) do
               begin
                  MemTmp.Seek((ii * w) div 2, soFromBeginning);
                  MemInput.CopyFrom(MemTmp, w div 2);
               end;
            finally
               MemTmp.Free;
            end;

            // get palette
            ss := '    dc.' + formatasm + '  ';
            MemInput.Seek(54, soFromBeginning);
            for ii := 0 to 15 do
            begin
               // get color
               MemInput.Read(jj, 4);
               ss := ss + '0x' + IntToHex(rgb24_to_vdp_color(jj), 4) + ', ';
            end;

            FileOutput.Add(delete_last(ss, 2));
            offset := (2 * 2) + (2 * 16);
         end;

         data := 0;
         while (offset < len) do
         begin
            ss := '    dc.' + formatasm + '  ';
            for ii := 0 to ((16 div formatint) - 1)  do
            begin
               if (MemInput.Read(data, formatint) <> 0) then
                  ss := ss + '0x' + IntToHex(data, 2 * formatint) + ', '
               else
                  ss := ss + '0x' + IntToHex(nullfill, 2 * formatint) + ', ';
            end;

            Inc(offset, 16);
            FileOutput.Add(delete_last(ss, 2));
         end;
      end else FileOutput.Add('    dc.' + formatasm + '  0x00');

      FileOutput.SaveToFile(ChangeFileExt(FileNameOut, '.s'));

      FileOutput.Clear;

      ss := '_' + UpperCase(shortname) + '_H_';
      FileOutput.Add('#ifndef ' + ss);
      FileOutput.Add('#define ' + ss);
      FileOutput.Add('');
      FileOutput.Add('extern const ' + format + shortname + '[0x' + IntToHex(len div formatint, 4) + '];');
      FileOutput.Add('');
      FileOutput.Add('#endif // ' + ss);

      FileOutput.SaveToFile(ChangeFileExt(FileNameOut, '.h'));
   finally
      FileOutput.Free;
      MemInput.Free;
   end;
end.
