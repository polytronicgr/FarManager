/*
flplugin.cpp

�������� ������ - ������ � ���������

*/

/* Revision: 1.13 06.07.2001 $ */

/*
Modify:
  06.07.2001 IS
    + �������� ������ ��������� � PluginSetSelection � � PluginClearSelection
  14.06.2001 SVS
    - ��������� �� ������: "������� ������������� some.foo.rar � ������� some\"
  17.05.2001 SVS
    ! ������� ����������� ����� ���������� (����� doxygen ����� �� ������� :-)
  06.05.2001 DJ
    ! �������� #include
  29.04.2001 ��
    + ��������� NWZ �� ����������
  26.04.2001 DJ
    - � ProcessHostFile() �� ����������� OPM_TOPLEVEL
  04.01.2001 SVS
    ! TranslateKeyToVK() -> keyboard.cpp
  11.11.2000 SVS
    ! FarMkTemp() - ������� (��� ������ - �� ������, �� ��� �� ������� :-(((
  11.11.2000 SVS
    ! ���������� ����������� FarMkTemp()
  08.09.2000 SVS
    + ������� � FileList::TranslateKeyToVK ��� ����������
      KEY_SHIFTDEL, KEY_ALTSHIFTDEL, KEY_CTRLSHIFTDEL
  23.07.2000 SVS
    + ������� (FileList::TranslateKeyToVK):
       Ctrl- Shift- Alt- CtrlShift- AltShift- CtrlAlt- Apps :-)
       KEY_LWIN (VK_LWIN), KEY_RWIN (VK_RWIN)
  13.07.2000 SVS
    ! ��������� ��������� ��� ������������� new/delete/realloc
  11.07.2000 SVS
    ! ��������� ��� ����������� ���������� ��� BC & VC
  25.06.2000 SVS
    ! ���������� Master Copy
    ! ��������� � �������� ���������������� ������
*/

#include "headers.hpp"
#pragma hdrstop

#include "filelist.hpp"
#include "plugin.hpp"
#include "global.hpp"
#include "fn.hpp"
#include "filepanels.hpp"
#include "history.hpp"
#include "ctrlobj.hpp"

void FileList::PushPlugin(HANDLE hPlugin,char *HostFile)
{
  DeleteAllDataToDelete();
  PluginsStack=(struct PluginsStackItem *)realloc(PluginsStack,(PluginsStackSize+1)*sizeof(*PluginsStack));
  PluginsStack[PluginsStackSize].hPlugin=hPlugin;
  strcpy(PluginsStack[PluginsStackSize].HostFile,HostFile);
  PluginsStack[PluginsStackSize].Modified=FALSE;
  PluginsStack[PluginsStackSize].PrevViewMode=ViewMode;
  PluginsStack[PluginsStackSize].PrevSortMode=SortMode;
  PluginsStack[PluginsStackSize].PrevSortOrder=SortOrder;
  PluginsStackSize++;
}


int FileList::PopPlugin(int EnableRestoreViewMode)
{
  DeleteAllDataToDelete();
  if (PluginsStackSize==0)
  {
    PanelMode=NORMAL_PANEL;
    return(FALSE);
  }
  PluginsStackSize--;
  if (EnableRestoreViewMode)
  {
    struct OpenPluginInfo Info;
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
    if (Info.StartPanelMode)
      SetViewMode(PluginsStack[PluginsStackSize].PrevViewMode);
    if (Info.StartSortMode)
    {
      SortMode=PluginsStack[PluginsStackSize].PrevSortMode;
      SortOrder=PluginsStack[PluginsStackSize].PrevSortOrder;
    }
  }
  CtrlObject->Plugins.ClosePlugin(hPlugin);
  if (PluginsStackSize>0)
  {
    hPlugin=PluginsStack[PluginsStackSize-1].hPlugin;
    if (PluginsStack[PluginsStackSize].Modified)
    {
      struct PluginPanelItem PanelItem;
      char SaveDir[NM];
      GetCurrentDirectory(sizeof(SaveDir),SaveDir);
      if (FileNameToPluginItem(PluginsStack[PluginsStackSize].HostFile,&PanelItem))
        CtrlObject->Plugins.PutFiles(hPlugin,&PanelItem,1,FALSE,0);
      else
      {
        memset(&PanelItem,0,sizeof(PanelItem));
        strncpy(PanelItem.FindData.cFileName,PointToName(PluginsStack[PluginsStackSize].HostFile),NM);
        CtrlObject->Plugins.DeleteFiles(hPlugin,&PanelItem,1,0);
      }
      chdir(SaveDir);
    }
    struct OpenPluginInfo Info;
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
    if ((Info.Flags & OPIF_REALNAMES)==0)
      DeleteFileWithFolder(PluginsStack[PluginsStackSize].HostFile);
  }
  else
    PanelMode=NORMAL_PANEL;
  PluginsStack=(struct PluginsStackItem *)realloc(PluginsStack,PluginsStackSize*sizeof(*PluginsStack));
  if (EnableRestoreViewMode)
    CtrlObject->Cp()->RedrawKeyBar();
  return(TRUE);
}


int FileList::FileNameToPluginItem(char *Name,PluginPanelItem *pi)
{
  char TempDir[NM],*ChPtr;
  strcpy(TempDir,Name);
  if ((ChPtr=strrchr(TempDir,'\\'))==NULL)
    return(FALSE);
  *ChPtr=0;
  chdir(TempDir);
  memset(pi,0,sizeof(*pi));
  HANDLE FindHandle;
  FindHandle=FindFirstFile(Name,&pi->FindData);
  if (FindHandle==INVALID_HANDLE_VALUE)
    return(FALSE);
  FindClose(FindHandle);
  return(TRUE);
}


void FileList::FileListToPluginItem(struct FileListItem *fi,struct PluginPanelItem *pi)
{
  strcpy(pi->FindData.cFileName,fi->Name);
  strcpy(pi->FindData.cAlternateFileName,fi->ShortName);
  pi->FindData.nFileSizeHigh=fi->UnpSizeHigh;
  pi->FindData.nFileSizeLow=fi->UnpSize;
  pi->FindData.dwReserved0=pi->FindData.dwReserved1=0;
  pi->PackSizeHigh=fi->PackSizeHigh;
  pi->PackSize=fi->PackSize;
  pi->FindData.dwFileAttributes=fi->FileAttr;
  pi->FindData.ftLastWriteTime=fi->WriteTime;
  pi->FindData.ftCreationTime=fi->CreationTime;
  pi->FindData.ftLastAccessTime=fi->AccessTime;
  pi->NumberOfLinks=fi->NumberOfLinks;
  pi->Flags=fi->UserFlags;
  if (fi->Selected)
    pi->Flags|=PPIF_SELECTED;
  pi->CustomColumnData=fi->CustomColumnData;
  pi->CustomColumnNumber=fi->CustomColumnNumber;
  pi->Description=fi->DizText;
  if (fi->UserData && (fi->UserFlags & PPIF_USERDATA))
  {
    DWORD Size=*(DWORD *)fi->UserData;
    /* $ 13.07.2000 SVS
       ������� new �� malloc
    */
    pi->UserData=(DWORD)malloc(Size);
    /* SVS $ */
    memcpy((void *)pi->UserData,(void *)fi->UserData,Size);
  }
  else
    pi->UserData=fi->UserData;
  pi->Reserved[0]=pi->Reserved[1]=pi->Reserved[2]=0;
  pi->Owner=NULL;
}


void FileList::PluginToFileListItem(struct PluginPanelItem *pi,struct FileListItem *fi)
{
  strncpy(fi->Name,pi->FindData.cFileName,sizeof(fi->Name));
  strncpy(fi->ShortName,pi->FindData.cAlternateFileName,sizeof(fi->ShortName));
  strncpy(fi->Owner,NullToEmpty(pi->Owner),sizeof(fi->Owner));
  if (pi->Description)
  {
    fi->DizText=new char[strlen(pi->Description)+1];
    strcpy(fi->DizText,pi->Description);
    fi->DeleteDiz=TRUE;
  }
  else
    fi->DizText=NULL;
  fi->UnpSizeHigh=pi->FindData.nFileSizeHigh;
  fi->UnpSize=pi->FindData.nFileSizeLow;
  fi->PackSizeHigh=pi->PackSizeHigh;
  fi->PackSize=pi->PackSize;
  fi->FileAttr=pi->FindData.dwFileAttributes;
  fi->WriteTime=pi->FindData.ftLastWriteTime;
  fi->CreationTime=pi->FindData.ftCreationTime;
  fi->AccessTime=pi->FindData.ftLastAccessTime;
  fi->NumberOfLinks=pi->NumberOfLinks;
  fi->UserFlags=pi->Flags;

  if (pi->UserData && (pi->Flags & PPIF_USERDATA))
  {
    DWORD Size=*(DWORD *)pi->UserData;
    /* $ 13.07.2000 SVS
       ������� new �� malloc
    */
    fi->UserData=(DWORD)malloc(Size);
    /* SVS $ */
    memcpy((void *)fi->UserData,(void *)pi->UserData,Size);
  }
  else
    fi->UserData=pi->UserData;
  if (pi->CustomColumnNumber>0)
  {
    fi->CustomColumnData=new LPSTR[pi->CustomColumnNumber];
    for (int I=0;I<pi->CustomColumnNumber;I++)
      if (pi->CustomColumnData!=NULL && pi->CustomColumnData[I]!=NULL)
      {
        fi->CustomColumnData[I]=new char[strlen(pi->CustomColumnData[I])+1];
        strcpy(fi->CustomColumnData[I],pi->CustomColumnData[I]);
      }
      else
        fi->CustomColumnData[I]="";
  }
  fi->CustomColumnNumber=pi->CustomColumnNumber;
}


HANDLE FileList::OpenPluginForFile(char *FileName)
{
  const int MaxRead=0x20000;
  char *Buffer=new char[MaxRead];
  SetCurPath();
  FILE *ProcessFile=fopen(FileName,"rb");
  /* $ 13.07.2000 SVS
     ������ delete -> delete[]
  */
  if (ProcessFile==NULL)
  {
    delete[] Buffer;
    return(INVALID_HANDLE_VALUE);
  }
  int ReadSize=fread(Buffer,1,MaxRead,ProcessFile);
  fclose(ProcessFile);
  CtrlObject->Cp()->GetAnotherPanel(this)->CloseFile();
  HANDLE hNewPlugin=CtrlObject->Plugins.OpenFilePlugin(FileName,(unsigned char *)Buffer,ReadSize);
  delete[] Buffer;
  /* SVS $ */
  return(hNewPlugin);
}


void FileList::CreatePluginItemList(struct PluginPanelItem *(&ItemList),int &ItemNumber)
{
  long SaveSelPosition=GetSelPosition;
  char SelName[NM];
  int FileAttr;
  ItemNumber=0;
  ItemList=new PluginPanelItem[SelFileCount+1];
  GetSelName(NULL,FileAttr);
  if (ItemList!=NULL)
    while (GetSelName(SelName,FileAttr))
      if (((FileAttr & FA_DIREC)==0 || strcmp(SelName,"..")!=0)
          && LastSelPosition>=0 && LastSelPosition<FileCount)
      {
        FileListToPluginItem(ListData+LastSelPosition,ItemList+ItemNumber);
        ItemNumber++;
      }
  GetSelPosition=SaveSelPosition;
}


void FileList::DeletePluginItemList(struct PluginPanelItem *(&ItemList),int &ItemNumber)
{
  for (int I=0;I<ItemNumber;I++)
    if (ItemList[I].Flags & PPIF_USERDATA)
      free((void *)ItemList[I].UserData);
  delete[] ItemList;
}


void FileList::PluginDelete()
{
  struct PluginPanelItem *ItemList;
  int ItemNumber;
  SaveSelection();
  CreatePluginItemList(ItemList,ItemNumber);
  if (ItemList!=NULL && ItemNumber>0)
  {
    if (CtrlObject->Plugins.DeleteFiles(hPlugin,ItemList,ItemNumber,0))
    {
      SetPluginModified();
      PutDizToPlugin(this,ItemList,ItemNumber,TRUE,FALSE,NULL,&Diz);
    }
    DeletePluginItemList(ItemList,ItemNumber);
    Update(UPDATE_KEEP_SELECTION);
    Redraw();
    Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
    AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
    AnotherPanel->Redraw();
  }
}


void FileList::PutDizToPlugin(FileList *DestPanel,struct PluginPanelItem *ItemList,
                              int ItemNumber,int Delete,int Move,DizList *SrcDiz,
                              DizList *DestDiz)
{
  struct OpenPluginInfo Info;
  CtrlObject->Plugins.GetOpenPluginInfo(DestPanel->hPlugin,&Info);
  if (*DestPanel->PluginDizName==0 && Info.DescrFilesNumber>0)
    strcpy(DestPanel->PluginDizName,Info.DescrFiles[0]);
  if ((Opt.Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED && IsDizDisplayed() ||
      Opt.Diz.UpdateMode==DIZ_UPDATE_ALWAYS) && *DestPanel->PluginDizName &&
      (Info.HostFile==NULL || *Info.HostFile==0 || DestPanel->GetModalMode() ||
      GetFileAttributes(Info.HostFile)!=0xFFFFFFFF))
  {
    CtrlObject->Cp()->LeftPanel->ReadDiz();
    CtrlObject->Cp()->RightPanel->ReadDiz();

    if (DestPanel->GetModalMode())
      DestPanel->ReadDiz();

    int DizPresent=FALSE;
    for (int I=0;I<ItemNumber;I++)
      if (ItemList[I].Flags & PPIF_PROCESSDESCR)
      {
        char *Name=ItemList[I].FindData.cFileName;
        char *ShortName=ItemList[I].FindData.cAlternateFileName;
        int Code;
        if (Delete)
          Code=DestDiz->DeleteDiz(Name,ShortName);
        else
        {
          Code=SrcDiz->CopyDiz(Name,ShortName,Name,ShortName,DestDiz);
          if (Code && Move)
            SrcDiz->DeleteDiz(Name,ShortName);
        }
        if (Code)
          DizPresent=TRUE;
      }
    if (DizPresent)
    {
      char TempDir[NM],DizName[NM];
      sprintf(TempDir,"%s%s",Opt.TempPath,FarTmpXXXXXX);
      if (mktemp(TempDir)!=NULL && CreateDirectory(TempDir,NULL))
      //if (FarMkTemp(TempDir,"Far")!=NULL && CreateDirectory(TempDir,NULL))
      {
        char SaveDir[NM];
        GetCurrentDirectory(sizeof(SaveDir),SaveDir);
        sprintf(DizName,"%s\\%s",TempDir,DestPanel->PluginDizName);
        DestDiz->Flush("",DizName);
        if (Move)
          SrcDiz->Flush("",NULL);
        struct PluginPanelItem PanelItem;
        if (FileNameToPluginItem(DizName,&PanelItem))
          CtrlObject->Plugins.PutFiles(DestPanel->hPlugin,&PanelItem,1,FALSE,OPM_SILENT|OPM_DESCR);
        else
          if (Delete)
          {
            PluginPanelItem pi;
            memset(&pi,0,sizeof(pi));
            strcpy(pi.FindData.cFileName,DestPanel->PluginDizName);
            CtrlObject->Plugins.DeleteFiles(DestPanel->hPlugin,&pi,1,OPM_SILENT);
          }
        chdir(SaveDir);
        DeleteFileWithFolder(DizName);
      }
    }
  }
}


void FileList::PluginGetFiles(char *DestPath,int Move)
{
  struct PluginPanelItem *ItemList;
  int ItemNumber;
  SaveSelection();
  CreatePluginItemList(ItemList,ItemNumber);
  if (ItemList!=NULL && ItemNumber>0)
  {
    int GetCode=CtrlObject->Plugins.GetFiles(hPlugin,ItemList,ItemNumber,Move,DestPath,0);
    if (Opt.Diz.UpdateMode==DIZ_UPDATE_IF_DISPLAYED && IsDizDisplayed() ||
        Opt.Diz.UpdateMode==DIZ_UPDATE_ALWAYS)
    {
      DizList DestDiz;
      int DizFound=FALSE;
      for (int I=0;I<ItemNumber;I++)
        if (ItemList[I].Flags & PPIF_PROCESSDESCR)
        {
          if (!DizFound)
          {
            CtrlObject->Cp()->LeftPanel->ReadDiz();
            CtrlObject->Cp()->RightPanel->ReadDiz();
            DestDiz.Read(DestPath);
            DizFound=TRUE;
          }
          char *Name=ItemList[I].FindData.cFileName;
          char *ShortName=ItemList[I].FindData.cAlternateFileName;
          CopyDiz(Name,ShortName,Name,Name,&DestDiz);
        }
      DestDiz.Flush(DestPath);
    }
    if (GetCode==1)
    {
      if (!ReturnCurrentFile)
        ClearSelection();
      if (Move)
      {
        SetPluginModified();
        PutDizToPlugin(this,ItemList,ItemNumber,TRUE,FALSE,NULL,&Diz);
      }
    }
    else
      if (!ReturnCurrentFile)
        PluginClearSelection(ItemList,ItemNumber);
    DeletePluginItemList(ItemList,ItemNumber);
    Update(UPDATE_KEEP_SELECTION);
    Redraw();
    Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
    AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
    AnotherPanel->Redraw();
  }
}


void FileList::PluginToPluginFiles(int Move)
{
  struct PluginPanelItem *ItemList;
  int ItemNumber;
  Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
  char TempDir[NM];
  if (AnotherPanel->GetMode()!=PLUGIN_PANEL)
    return;
  FileList *AnotherFilePanel=(FileList *)AnotherPanel;

  strcpy(TempDir,Opt.TempPath);
  strcat(TempDir,FarTmpXXXXXX);
  if (mktemp(TempDir)==NULL)
  //if(!FarMkTemp(TempDir,"Far"))
    return;
  SaveSelection();
  CreateDirectory(TempDir,NULL);
  CreatePluginItemList(ItemList,ItemNumber);
  if (ItemList!=NULL && ItemNumber>0)
  {
    if (CtrlObject->Plugins.GetFiles(hPlugin,ItemList,ItemNumber,FALSE,TempDir,OPM_SILENT)==1)
    {
      char SaveDir[NM];
      GetCurrentDirectory(sizeof(SaveDir),SaveDir);
      chdir(TempDir);
      if (CtrlObject->Plugins.PutFiles(AnotherFilePanel->hPlugin,ItemList,ItemNumber,FALSE,0)==1)
      {
        if (!ReturnCurrentFile)
          ClearSelection();
        AnotherPanel->SetPluginModified();
        PutDizToPlugin(AnotherFilePanel,ItemList,ItemNumber,FALSE,FALSE,&Diz,&AnotherFilePanel->Diz);
        if (Move)
          if (CtrlObject->Plugins.DeleteFiles(hPlugin,ItemList,ItemNumber,OPM_SILENT))
          {
            SetPluginModified();
            PutDizToPlugin(this,ItemList,ItemNumber,TRUE,FALSE,NULL,&Diz);
          }
      }
      else
        if (!ReturnCurrentFile)
          PluginClearSelection(ItemList,ItemNumber);
      chdir(SaveDir);
    }
    DeleteDirTree(TempDir);
    DeletePluginItemList(ItemList,ItemNumber);
    Update(UPDATE_KEEP_SELECTION);
    Redraw();
    if (PanelMode==PLUGIN_PANEL)
      AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
    else
      AnotherPanel->Update(UPDATE_KEEP_SELECTION);
    AnotherPanel->Redraw();
  }
}


void FileList::PluginHostGetFiles()
{
  Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
  char DestPath[NM],SelName[NM],*ExtPtr;
  int FileAttr;

  SaveSelection();

  GetSelName(NULL,FileAttr);
  if (!GetSelName(SelName,FileAttr))
    return;

  AnotherPanel->GetCurDir(DestPath);
  if ((!AnotherPanel->IsVisible() || AnotherPanel->GetType()!=FILE_PANEL) &&
      SelFileCount==0 || *DestPath==0)
  {
    strcpy(DestPath,PointToName(SelName));
    // SVS: � ����� ����� ����� ����� ����� � ������?
    if ((ExtPtr=strrchr(DestPath,'.'))!=NULL)
      *ExtPtr=0;
  }

  int OpMode=OPM_TOPLEVEL,ExitLoop=FALSE;
  GetSelName(NULL,FileAttr);
  while (!ExitLoop && GetSelName(SelName,FileAttr))
  {
    HANDLE hCurPlugin;
    if ((hCurPlugin=OpenPluginForFile(SelName))!=INVALID_HANDLE_VALUE &&
        hCurPlugin!=(HANDLE)-2)
    {
      struct PluginPanelItem *ItemList;
      int ItemNumber;
      if (CtrlObject->Plugins.GetFindData(hCurPlugin,&ItemList,&ItemNumber,0))
      {
        ExitLoop=CtrlObject->Plugins.GetFiles(hCurPlugin,ItemList,ItemNumber,FALSE,DestPath,OpMode)!=1;
        if (!ExitLoop)
          ClearLastGetSelection();
        CtrlObject->Plugins.FreeFindData(hCurPlugin,ItemList,ItemNumber);
        OpMode|=OPM_SILENT;
      }
      CtrlObject->Plugins.ClosePlugin(hCurPlugin);
    }
  }
  Update(UPDATE_KEEP_SELECTION);
  Redraw();
  AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
  AnotherPanel->Redraw();
}


void FileList::PluginPutFilesToNew()
{
  HANDLE hNewPlugin=CtrlObject->Plugins.OpenFilePlugin(NULL,NULL,0);
  if (hNewPlugin!=INVALID_HANDLE_VALUE && hNewPlugin!=(HANDLE)-2)
  {
    FileList TmpPanel;
    TmpPanel.SetPluginMode(hNewPlugin,"");
    TmpPanel.SetModalMode(TRUE);
    int PrevFileCount=FileCount;
    PluginPutFilesToAnother(FALSE,&TmpPanel);
    if (FileCount==PrevFileCount+1)
    {
      int LastPos=0;
      for (int I=1;I<FileCount;I++)
        if (CompareFileTime(&ListData[I].CreationTime,&ListData[LastPos].CreationTime)==1)
          LastPos=I;
      CurFile=LastPos;
      Redraw();
    }
  }
}


void FileList::PluginPutFilesToAnother(int Move,Panel *AnotherPanel)
{
  if (AnotherPanel->GetMode()!=PLUGIN_PANEL)
    return;
  FileList *AnotherFilePanel=(FileList *)AnotherPanel;
  struct PluginPanelItem *ItemList;
  int ItemNumber;
  SaveSelection();
  CreatePluginItemList(ItemList,ItemNumber);
  if (ItemList!=NULL && ItemNumber>0)
  {
    SetCurPath();
    int PutCode=CtrlObject->Plugins.PutFiles(AnotherFilePanel->hPlugin,ItemList,ItemNumber,Move,0);
    if (PutCode==1)
    {
      if (!ReturnCurrentFile)
        ClearSelection();
      PutDizToPlugin(AnotherFilePanel,ItemList,ItemNumber,FALSE,Move,&Diz,&AnotherFilePanel->Diz);
      AnotherPanel->SetPluginModified();
    }
    else
      if (!ReturnCurrentFile)
        PluginClearSelection(ItemList,ItemNumber);
    DeletePluginItemList(ItemList,ItemNumber);
    Update(UPDATE_KEEP_SELECTION);
    Redraw();
    if (AnotherPanel==CtrlObject->Cp()->GetAnotherPanel(this))
    {
      AnotherPanel->Update(UPDATE_KEEP_SELECTION);
      AnotherPanel->Redraw();
    }
  }
}


void FileList::GetPluginInfo(struct PluginInfo *Info)
{
  memset(Info,0,sizeof(*Info));
  if (PanelMode==PLUGIN_PANEL)
  {
    struct PluginHandle *ph=(struct PluginHandle *)hPlugin;
    CtrlObject->Plugins.GetPluginInfo(ph->PluginNumber,Info);
  }
}


void FileList::GetOpenPluginInfo(struct OpenPluginInfo *Info)
{
  memset(Info,0,sizeof(*Info));
  if (PanelMode==PLUGIN_PANEL)
    CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,Info);
}


void FileList::ProcessHostFile()
{
  if (FileCount>0 && SetCurPath())
  {
    SaveSelection();
    int Done=FALSE;
    if (PanelMode==PLUGIN_PANEL && *PluginsStack[PluginsStackSize-1].HostFile)
    {
      struct PluginPanelItem *ItemList;
      int ItemNumber;
      CreatePluginItemList(ItemList,ItemNumber);
      Done=CtrlObject->Plugins.ProcessHostFile(hPlugin,ItemList,ItemNumber,0);
      if (Done)
        SetPluginModified();
      else
      {
        if (!ReturnCurrentFile)
          PluginClearSelection(ItemList,ItemNumber);
        Redraw();
      }
      DeletePluginItemList(ItemList,ItemNumber);
    }
    else
    {
      HANDLE hNewPlugin=OpenPluginForFile(ListData[CurFile].Name);
      if (hNewPlugin!=INVALID_HANDLE_VALUE && hNewPlugin!=(HANDLE)-2)
      {
        struct PluginPanelItem *ItemList;
        int ItemNumber;
        if (CtrlObject->Plugins.GetFindData(hNewPlugin,&ItemList,&ItemNumber,OPM_TOPLEVEL))
        {
          /* $ 26.04.2001 DJ
             � ProcessHostFile �� ����������� OPM_TOPLEVEL
          */
          Done=CtrlObject->Plugins.ProcessHostFile(hNewPlugin,ItemList,ItemNumber,OPM_TOPLEVEL);
          /* DJ $ */
          CtrlObject->Plugins.FreeFindData(hNewPlugin,ItemList,ItemNumber);
        }
        CtrlObject->Plugins.ClosePlugin(hNewPlugin);
      }
    }
    if (Done)
    {
      ClearSelection();
      Update(UPDATE_KEEP_SELECTION);
      Redraw();
      Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
      AnotherPanel->Update(UPDATE_KEEP_SELECTION|UPDATE_SECONDARY);
      AnotherPanel->Redraw();
    }
  }
}


void FileList::SetPluginMode(HANDLE hPlugin,char *PluginFile)
{
  if (PanelMode!=PLUGIN_PANEL)
    CtrlObject->FolderHistory->AddToHistory(CurDir,NULL,0);

  FileList::hPlugin=hPlugin;
  PushPlugin(hPlugin,PluginFile);
  PanelMode=PLUGIN_PANEL;
  struct OpenPluginInfo Info;
  CtrlObject->Plugins.GetOpenPluginInfo(hPlugin,&Info);
  if (Info.StartPanelMode)
    SetViewMode(VIEW_0+Info.StartPanelMode-'0');
  CtrlObject->Cp()->RedrawKeyBar();
  if (Info.StartSortMode)
  {
    SortMode=Info.StartSortMode-(SM_UNSORTED-UNSORTED);
    SortOrder=Info.StartSortOrder ? -1:1;
  }
  Panel *AnotherPanel=CtrlObject->Cp()->GetAnotherPanel(this);
  if (AnotherPanel->GetType()!=FILE_PANEL)
  {
    AnotherPanel->Update(UPDATE_KEEP_SELECTION);
    AnotherPanel->Redraw();
  }
}


void FileList::PluginGetPanelInfo(struct PanelInfo *Info)
{
  DeleteAllDataToDelete();
  Info->PanelItems=NULL;
  Info->ItemsNumber=0;
  Info->PanelItems=new PluginPanelItem[FileCount+1];
  if (Info->PanelItems!=NULL)
    for (int I=0;I<FileCount;I++)
    {
      FileListToPluginItem(ListData+I,Info->PanelItems+Info->ItemsNumber);
      Info->ItemsNumber++;
    }
  DataToDelete[DataToDeleteCount]=Info->PanelItems;
  DataSizeToDelete[DataToDeleteCount++]=Info->ItemsNumber;

  CreatePluginItemList(Info->SelectedItems,Info->SelectedItemsNumber);

  DataToDelete[DataToDeleteCount]=Info->SelectedItems;
  DataSizeToDelete[DataToDeleteCount++]=Info->SelectedItemsNumber;

  Info->CurrentItem=CurFile;
  Info->TopPanelItem=CurTopFile;

  char ColumnTypes[80],ColumnWidths[80];
  ViewSettingsToText(ViewSettings.ColumnType,ViewSettings.ColumnWidth,
                     ViewSettings.ColumnCount,ColumnTypes,ColumnWidths);
  strncpy(Info->ColumnTypes,ColumnTypes,sizeof(Info->ColumnTypes));
  strncpy(Info->ColumnWidths,ColumnWidths,sizeof(Info->ColumnWidths));
  Info->ShortNames=ShowShortNames;
}


void FileList::PluginSetSelection(struct PanelInfo *Info)
{
  /* $ 06.07.2001 IS �������� ������ ��������� */
  SaveSelection();
  /* IS $ */
  for (int I=0;I<FileCount && I<Info->ItemsNumber;I++)
  {
    int Selection=(Info->PanelItems[I].Flags & PPIF_SELECTED)!=0;
    Select(&ListData[I],Selection);
  }
  if (SelectedFirst)
    SortFileList(TRUE);
}


void FileList::ProcessPluginCommand()
{
  int Command=PluginCommand;
  PluginCommand=-1;
  if (PanelMode==PLUGIN_PANEL)
    switch(Command)
    {
      case FCTL_CLOSEPLUGIN:
        SetCurDir((char *)PluginParam,TRUE);
        Redraw();
        break;
    }
}

void FileList::SetPluginModified()
{
  if (PluginsStackSize>0)
    PluginsStack[PluginsStackSize-1].Modified=TRUE;
}


HANDLE FileList::GetPluginHandle()
{
  return(hPlugin);
}


int FileList::ProcessPluginEvent(int Event,void *Param)
{
  if (PanelMode==PLUGIN_PANEL)
    return(CtrlObject->Plugins.ProcessEvent(hPlugin,Event,Param));
  return(FALSE);
}


void FileList::PluginClearSelection(struct PluginPanelItem *ItemList,int ItemNumber)
{
  /* $ 06.07.2001 IS �������� ������ ��������� */
  SaveSelection();
  /* IS $ */
  int FileNumber=0,PluginNumber=0;
  while (PluginNumber<ItemNumber)
  {
    struct PluginPanelItem *CurPluginPtr=ItemList+PluginNumber;
    if ((CurPluginPtr->Flags & PPIF_SELECTED)==0)
    {
      while (LocalStricmp(CurPluginPtr->FindData.cFileName,ListData[FileNumber].Name)!=0)
        if (++FileNumber>=FileCount)
          return;
      Select(&ListData[FileNumber++],0);
    }
    PluginNumber++;
  }
}
