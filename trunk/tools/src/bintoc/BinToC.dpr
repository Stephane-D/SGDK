program BinToC;

{$APPTYPE CONSOLE}

uses
  SysUtils, Classes;


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



var
   bitmap: boolean;
   ii, jj: integer;
   data: cardinal;
   w, h: integer;
   len: integer;
   offset: integer;
   formatint: integer;
   format: string;
   ss: string;
   shortname: string;
   FileName: string;
   FileInput: TFileStream;
   FileOutput: TStringList;
   MemInput: TMemoryStream;
   MemTmp: TMemoryStream;
begin
   bitmap:= false;
   FileName:= UnixPathToDosPath(ParamStr(2));
   if (ParamStr(1) = '-u8') then begin
      format:= 'u8 ';
      formatint:= 1;
   end else if (ParamStr(1) = '-s8') then begin
      format:= 's8 ';
      formatint:= 1;
   end else if (ParamStr(1) = '-u16') then begin
      format:= 'u16 ';
      formatint:= 2;
   end else if (ParamStr(1) = '-s16') then begin
      format:= 's16 ';
      formatint:= 2;
   end else if (ParamStr(1) = '-u32') then begin
      format:= 'u32 ';
      formatint:= 4;
   end else if (ParamStr(1) = '-s32') then begin
      format:= 's32 ';
      formatint:= 4;
   end else if (ParamStr(1) = '-bmp') then begin
      bitmap:= true;
      format:= 'u16 ';
      formatint:= 2;
   end else begin
      format:= 'u8 ';
      formatint:= 1;
      FileName:= UnixPathToDosPath(ParamStr(1));
   end;

   if (UpperCase(ExtractFileExt(FileName)) = '.BMP') then begin
      bitmap:= true;
      format:= 'u16 ';
      formatint:= 2;
   end;

   if (not FileExists(FileName)) then MemInput:= nil
   else begin
      MemInput:= TMemoryStream.Create;
      FileInput:= TFileStream.Create(FileName, fmOpenRead or fmShareDenyWrite);
      // we use a memory copy of the file
      try
         MemInput.CopyFrom(FileInput, 0);
         MemInput.Seek(0, soFromBeginning);
      finally
         FileInput.Free;
      end;
   end;

   FileOutput:= TStringList.Create;
   try
      offset:= 0;
      if (not Assigned(MemInput)) then len:= 1
      else len:= MemInput.Size;

      if (bitmap) then Dec(len, 118 - ((2 * 2) + (2 * 16)));

      // adjust size for word/long boundary
      len:= (len + (formatint - 1)) and (not (formatint - 1));

      shortname:= ChangeFileExt(ExtractFileName(FileName), '');

      FileOutput.Add('#include "genesis.h"');
      FileOutput.Add('');
      FileOutput.Add('const ' + format + shortname + '[0x' + IntToHex(len div formatint, 4) + '] = {');

      if (Assigned(MemInput)) then begin
         if (bitmap) then begin
            MemInput.Seek(18, soFromBeginning);

            // get width
            MemInput.Read(w, 4);
            ss:= '0x' + IntToHex(w, 4) + ', ';
            // get height
            MemInput.Read(h, 4);
            ss:= ss + '0x' + IntToHex(h, 4) + ', ';

            FileOutput.Add(#9 + ss);

            MemTmp:= TMemoryStream.Create;
            try
               // reverse lines
               for ii:= (h - 1) downto 0 do begin
                  MemInput.Seek(118 + ((ii * h) div 2), soFromBeginning);
                  for jj:= 0 to ((w div 8) - 1) do begin
                     MemInput.Read(data, 4);
                     // reverse high and low byte
                     data:= ((data shr 24) and $000000FF) or
                            ((data shr 08) and $0000FF00) or
                            ((data shl 08) and $00FF0000) or
                            ((data shl 24) and $FF000000);
                     data:= ((data shr 16) and $0000FFFF) or
                            ((data shl 16) and $FFFF0000);
                     MemTmp.Write(data, 4);
                  end;
               end;

               // erase old lines
               MemInput.Seek(118, soFromBeginning);
               for ii:= 0 to (h - 1) do begin
                  MemTmp.Seek((ii * h) div 2, soFromBeginning);
                  MemInput.CopyFrom(MemTmp, h div 2);
               end;
            finally
               MemTmp.Free;
            end;

            // get palette
            ss:= '';
            MemInput.Seek(54, soFromBeginning);
            for ii:= 0 to 15 do begin
               // get color
               MemInput.Read(jj, 4);
               ss:= ss + '0x' + IntToHex(rgb24_to_vdp_color(jj), 4) + ', ';
            end;

            FileOutput.Add(#9 + ss);
            offset:= (2 * 2) + (2 * 16);
         end;

         while (offset < len) do begin
            ss:= '';
            for ii:= 0 to ((16 div formatint) - 1)  do begin
               if (MemInput.Read(jj, formatint) <> 0) then ss:= ss + '0x' + IntToHex(jj, 2 * formatint) + ', '
            end;

            Inc(offset, 16);
            FileOutput.Add(#9 + ss);
         end;
      end else FileOutput.Add(#9 + '0x00');

      FileOutput.Add('};');
      FileOutput.SaveToFile(ChangeFileExt(FileName, '.c'));

      FileOutput.Clear;

      ss:= '_' + UpperCase(shortname) + '_H_';
      FileOutput.Add('#ifndef ' + ss);
      FileOutput.Add('#define ' + ss);
      FileOutput.Add('');
      FileOutput.Add('extern const ' + format + shortname + '[0x' + IntToHex(len div formatint, 4) + '];');
      FileOutput.Add('');
      FileOutput.Add('#endif // ' + ss);

      FileOutput.SaveToFile(ChangeFileExt(FileName, '.h'));
   finally
      FileOutput.Free;
      MemInput.Free;
   end;
end.
