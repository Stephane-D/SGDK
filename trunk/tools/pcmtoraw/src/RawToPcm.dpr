program RawToPcm;

{$APPTYPE CONSOLE}

uses
  SysUtils, Classes;

const
   delta_tab: array[0..15] of shortint =
   (-34,-21,-13,-8,-5,-3,-2,-1,0,1,2,3,5,8,13,21);


function UnixPathToDosPath(path: string): string;
begin
   result:= StringReplace(path, '/', '\', [rfReplaceAll]);
end;


var
   sample_over: integer;
   sample_loss: integer;

   function GetBestDelta(pWantedLevel: integer; pCurLevel: integer): shortint;
   var
      i: integer;
      ind: integer;
      diff: integer;
      mindiff: integer;
      wdelta: integer;
   begin
      wdelta := pWantedLevel - pCurLevel;
      ind := 0;
      mindiff := Abs(wdelta - delta_tab[ind]);

      for i := 1 to High(delta_tab) do
      begin
         diff := Abs(wdelta - delta_tab[i]);
         if (diff < mindiff) then
         begin
            mindiff := diff;
            ind := i;
         end;
      end;

      if (mindiff <> 0) then
      begin
         Inc(sample_over);
         Inc(sample_loss, mindiff);
      end;

      // check for overflow
      if (delta_tab[ind] + pCurLevel > High(result)) then
         result := Pred(ind)
      else if (delta_tab[ind] + pCurLevel < Low(result)) then
         result := Succ(ind)
      else
         result := ind;
   end;

var
   ii: integer;
   diff_ind: integer;
   curlevel: shortint;
   data_sb: shortint;
   diff_sb: shortint;
   out_sb: shortint;
   FileName: string;
   FileNameOut: string;
   FileInput: TFileStream;
   FileOutput: TFileStream;
   FileOutputLoss: TFileStream;
   MemInput: TMemoryStream;
begin
   // default
   FileName := '';
   FileNameOut := '';

   // parse parmeters
   ii := 1;
   while (ii <= ParamCount) do
   begin
      if (FileName = '') then FileName:= UnixPathToDosPath(ParamStr(ii))
      else if (FileNameOut = '') then FileNameOut:= UnixPathToDosPath(ParamStr(ii));

      Inc(ii);
   end;

   // exit
   if (not FileExists(FileName)) then exit;

   if (FileNameOut = '') then FileNameOut := ChangeFileExt(FileName, '.raw');

   MemInput := TMemoryStream.Create;
   FileInput := TFileStream.Create(FileName, fmOpenRead or fmShareDenyWrite);
   FileOutput := TFileStream.Create(FileNameOut, fmCreate or fmShareExclusive);
   FileOutputLoss := TFileStream.Create('loss', fmCreate or fmShareExclusive);
   try
      // we use a memory copy of file for faster access
      MemInput.CopyFrom(FileInput, 0);

      sample_over := 0;
      sample_loss := 0;

      ii := 0;
      curlevel := 0;
      MemInput.Seek(0, soFromBeginning);
      while (ii < MemInput.Size) do
      begin
         MemInput.Read(data_sb, 1);
         diff_ind := GetBestDelta(data_sb, curlevel);
         diff_sb := delta_tab[diff_ind];
         curlevel := curlevel + diff_sb;
         FileOutputLoss.Write(curlevel, 1);

         out_sb := diff_ind;

         MemInput.Read(data_sb, 1);
         diff_ind := GetBestDelta(data_sb, curlevel);
         diff_sb := delta_tab[diff_ind];
         curlevel := curlevel + diff_sb;
         FileOutputLoss.Write(curlevel, 1);

         out_sb := out_sb or (diff_ind shl 4);

         FileOutput.Write(out_sb, 1);

         Inc(ii, 2);
      end;


   finally
      FileOutputLoss.Free;
      FileOutput.Free;
      FileInput.Free;
      MemInput.Free;
   end;
end.
