program SizeBnd;

{$APPTYPE CONSOLE}

uses
  SysUtils, Classes;


  function UnixPathToDosPath(path: string): string;
begin
   result:= StringReplace(path, '/', '\', [rfReplaceAll]);
end;


var
   ii: integer;
   needed: integer;
   sizealign: integer;
   nullfill: integer;
   FileName: string;
   FileInput: TFileStream;
begin
   // default
   FileName := '';
   sizealign := 256;
   nullfill := 0;

   // parse parmeters
   ii := 1;
   while (ii <= ParamCount) do
   begin
      if (ParamStr(ii) = '-sizealign') then begin
         Inc(ii);
         sizealign := StrToIntDef(ParamStr(ii), 1);
      end else if (ParamStr(ii) = '-nullfill') then begin
         Inc(ii);
         nullfill := StrToIntDef(ParamStr(ii), 0);
      end else if (FileName = '') then FileName := UnixPathToDosPath(ParamStr(ii));

      Inc(ii);
   end;

   if FileExists(FileName) then
   begin
      FileInput := TFileStream.Create(FileName, fmOpenReadWrite or fmShareExclusive);
      try
         // calculate how many extra byte are needed
         needed := FileInput.Size and Pred(sizealign);
         if (needed <> 0) then
         begin
            needed := sizealign - needed;

            // go to end of file
            FileInput.Seek(0, soEnd);
            // complete missing bytes
            while (needed > 0) do
            begin
               FileInput.Write(nullfill, 1);
               Dec(needed);
            end;
         end;
      finally
         FileInput.Free;
      end;
   end;
end.
