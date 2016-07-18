#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "piXmlLogger.h"

namespace piLibs {

piXmlLogger::piXmlLogger() :piLogger()
{
}
piXmlLogger::~piXmlLogger()
{
}

static void printHeader(FILE *fp, const piLogStartInfo *info)
{
	fwprintf(fp, L"<?xml version=\"1.0\"?>\n");
	fwprintf(fp, L"<?xml-stylesheet type=\"text/xsl\" href=\"ComplexXSLT.xsl\" ?>\n");
	fwprintf(fp, L"\n");
	fwprintf(fp, L"\n");
	fwprintf(fp, L"<DebugSesion>\n");
	fwprintf(fp, L"\n");
	fwprintf(fp, L"<Info>\n");
	fwprintf(fp, L"  <Date>%s</Date>\n", info->date);
	fwprintf(fp, L"  <Memory>%d / %d Megabytes</Memory>", info->free_memory_MB, info->total_memory_MB);
	fwprintf(fp, L"  <CPU>\n");
	fwprintf(fp, L"    <Processor>%s</Processor>\n", info->processor);
	fwprintf(fp, L"    <Units>%d</Units>\n", info->number_cpus);
	fwprintf(fp, L"    <Speed>%d Mhz</Speed>\n", info->mhz);
	fwprintf(fp, L"    <CPU/>\n");
	fwprintf(fp, L"    <OS>%s</OS>\n", info->os);
	fwprintf(fp, L"  <GPU>\n");
	fwprintf(fp, L"    <Vendor>%s</Vendor>\n", info->gpuVendor);
	fwprintf(fp, L"    <Model>%s</Model>\n", info->gpuModel);
	fwprintf(fp, L"  </GPU>\n");
	fwprintf(fp, L"  <VRam>%d Megabytes</GPU>\n", info->total_videomemory_MB);
	fwprintf(fp, L"  <Screen>%d x %d</Screen>\n", info->mScreenResolution[0], info->mScreenResolution[1]);
	fwprintf(fp, L"  <Multitouch>%d</Multitouch>\n", info->mIntegratedMultitouch);
	fwprintf(fp, L"</Info>\n");
	fwprintf(fp, L"\n");
	fwprintf(fp, L"<Events>\n");

	fflush(fp);
}

bool piXmlLogger::Init(const wchar_t *path, const piLogStartInfo *info)
{
	//path
	mFp = _wfopen(path, L"wt");
	if (!mFp)
		return false;

	printHeader(mFp, info);

	return true;
}

void piXmlLogger::End(void)
{
	fprintf(mFp, "</DebugSesion>\n");
	fclose(mFp);
}

static const wchar_t *stype[5] = { L"Error", L"Warning", L"Assume", L"Message", L"Debug" };

void piXmlLogger::Printf(int messageId, int threadId, const wchar_t *file, const wchar_t *func, int line, int type, const wchar_t *str)
{
	if (!mFp) return;

	fwprintf(mFp, L"    <Event id=\"%d\">\n", messageId);
	fwprintf(mFp, L"      <Type>%s</Type>\n", stype[type - 1]);
	fwprintf(mFp, L"      <Th>%d</Th>\n", threadId);
	fwprintf(mFp, L"      <File>%s</File>\n", file);
	fwprintf(mFp, L"      <Func>%s</Func>\n", func);
	fwprintf(mFp, L"      <Line>%d</Line>\n", line);
	fwprintf(mFp, L"      <Text>%s</Text>\n", str);
	fwprintf(mFp, L"    </Event>\n");

	fflush(mFp);

}

}