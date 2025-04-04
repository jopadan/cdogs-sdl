/*
 Copyright (c) 2013-2017, 2025 Cong Xu
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
#include "yajl_utils.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>

static char *ReadFile(const char *filename);
yajl_val YAJLReadFile(const char *filename)
{
	yajl_val node = NULL;
	char errbuf[1024];
	char *buf = ReadFile(filename);
	if (buf == NULL)
	{
		fprintf(stderr, "Error reading JSON file '%s'\n", filename);
		goto bail;
	}
	node = yajl_tree_parse(buf, errbuf, sizeof errbuf);
	if (node == NULL)
	{
		fprintf(
			stderr, "Error parsing font JSON '%s': %s\n", filename, errbuf);
		goto bail;
	}

bail:
	free(buf);
	return node;
}
// Read a file into a dynamic buffer
static char *ReadFile(const char *filename)
{
	FILE *f = fopen(filename, "r");
	char *buf = NULL;
	if (f == NULL)
		goto bail;
	if (fseek(f, 0L, SEEK_END) != 0)
		goto bail;
	const long bufsize = ftell(f);
	if (bufsize == -1)
		goto bail;
	if (fseek(f, 0L, SEEK_SET) != 0)
		goto bail;
	buf = malloc(bufsize + 1);
	if (buf == NULL)
		goto bail;
	const size_t readlen = fread(buf, 1, bufsize, f);
	buf[readlen] = '\0';

bail:
	if (f)
	{
		fclose(f);
	}
	return buf;
}

#define YAJL_CHECK(func)                                                      \
	{                                                                         \
		yajl_gen_status _status = func;                                       \
		if (_status != yajl_gen_status_ok)                                    \
		{                                                                     \
			return _status;                                                   \
		}                                                                     \
	}

yajl_gen_status YAJLAddIntPair(yajl_gen g, const char *name, const int number)
{
	YAJL_CHECK(yajl_gen_string(g, (const unsigned char *)name, strlen(name)));
	YAJL_CHECK(yajl_gen_integer(g, number));
	return yajl_gen_status_ok;
}
yajl_gen_status YAJLAddBoolPair(yajl_gen g, const char *name, const bool value)
{
	YAJL_CHECK(yajl_gen_string(g, (const unsigned char *)name, strlen(name)));
	YAJL_CHECK(yajl_gen_bool(g, value));
	return yajl_gen_status_ok;
}
yajl_gen_status YAJLAddStringPair(yajl_gen g, const char *name, const char *s)
{
	YAJL_CHECK(yajl_gen_string(g, (const unsigned char *)name, strlen(name)));
	YAJL_CHECK(yajl_gen_string(
		g, (const unsigned char *)(s ? s : ""), s ? strlen(s) : 0));
	return yajl_gen_status_ok;
}
yajl_gen_status YAJLAddColorPair(yajl_gen g, const char *name, const color_t c)
{
	char buf[COLOR_STR_BUF];
	ColorStr(buf, c);
	return YAJLAddStringPair(g, name, buf);
}

bool YAJLTryLoadValue(yajl_val *node, const char *name)
{
	if (*node == NULL || !YAJL_IS_OBJECT(*node))
	{
		return false;
	}
	const char *path[] = {name, NULL};
	*node = yajl_tree_get(*node, path, yajl_t_any);
	return *node != NULL;
}
void YAJLBool(bool *value, yajl_val node, const char *name)
{
	if (!YAJLTryLoadValue(&node, name))
	{
		return;
	}
	*value = YAJL_IS_TRUE(node);
}
void YAJLInt(int *value, yajl_val node, const char *name)
{
	if (!YAJLTryLoadValue(&node, name) || !YAJL_IS_INTEGER(node))
	{
		return;
	}
	*value = (int)YAJL_GET_INTEGER(node);
}
void YAJLDouble(double *value, yajl_val node, const char *name)
{
	if (!YAJLTryLoadValue(&node, name) || !YAJL_IS_DOUBLE(node))
	{
		return;
	}
	*value = YAJL_GET_DOUBLE(node);
}
void YAJLVec2i(struct vec2i *value, yajl_val node, const char *name)
{
	if (!YAJLTryLoadValue(&node, name) || !YAJL_IS_ARRAY(node))
	{
		return;
	}
	*value = YAJL_GET_VEC2I(node);
}
void YAJLStr(char **value, yajl_val node, const char *name)
{
	if (!YAJLTryLoadValue(&node, name) || !YAJL_IS_STRING(node))
	{
		return;
	}
	CSTRDUP(*value, node->u.string);
}
char *YAJLGetStr(yajl_val node, const char *name)
{
	char *in = YAJL_GET_STRING(YAJLFindNode(node, name));
	CASSERT(in != NULL, "cannot get JSON string");
	char *out;
	CSTRDUP(out, in);
	return out;
}
/*
void LoadSoundFromNode(Mix_Chunk **value, json_t *node, const char *name)
{
	if (json_find_first_label(node, name) == NULL)
	{
		return;
	}
	if (!TryLoadValue(&node, name))
	{
		return;
	}
	*value = StrSound(node->text);
}
void LoadPic(const Pic **value, json_t *node, const char *name)
{
	if (json_find_first_label(node, name))
	{
		char *tmp = GetString(node, name);
		*value = PicManagerGetPic(&gPicManager, tmp);
		CFREE(tmp);
	}
}
void LoadBulletGuns(CArray *guns, json_t *node, const char *name)
{
	node = json_find_first_label(node, name);
	if (node == NULL || node->child == NULL)
	{
		return;
	}
	CArrayInit(guns, sizeof(const WeaponClass *));
	for (json_t *gun = node->child->child; gun; gun = gun->next)
	{
		const WeaponClass *wc = StrWeaponClass(gun->text);
		CArrayPushBack(guns, &g);
	}
}
 */
void YAJLLoadColor(color_t *c, yajl_val node, const char *name)
{
	if (!YAJLTryLoadValue(&node, name) || !YAJL_IS_STRING(node))
	{
		return;
	}
	*c = StrColor(node->u.string);
}
yajl_val YAJLFindNode(yajl_val node, const char *path)
{
	// max 256 levels
	const char *pathSplit[256];
	for (int i = 0; i < 256; i++)
		pathSplit[i] = NULL;
	char *pathCopy;
	CSTRDUP(pathCopy, path);
	char *pch = strtok(pathCopy, "/");
	int i = 0;
	yajl_val out = NULL;
	while (pch != NULL)
	{
		if (i == 256)
		{
			fprintf(stderr, "JSON path too long: '%s'\n", path);
			goto bail;
		}
		pathSplit[i] = pch;
		i++;
		pch = strtok(NULL, "/");
	}
	out = yajl_tree_get(node, pathSplit, yajl_t_any);

bail:
	CFREE(pathCopy);
	return out;
}

bool YAJLTrySaveJSONFile(yajl_gen g, const char *filename)
{
	const char *buf;
	size_t len;
	bool res = true;
	yajl_gen_get_buf(g, (const unsigned char **)&buf, &len);
	FILE *f = fopen(filename, "w");
	if (f == NULL)
	{
		LOG(LM_MAIN, LL_ERROR, "Unable to save %s\n", filename);
		res = false;
		goto bail;
	}
	fwrite(buf, 1, len, f);

#ifdef __EMSCRIPTEN__
	EM_ASM(
		// persist changes
		FS.syncfs(false, function(err) { assert(!err); }););
#endif

bail:
	if (f != NULL)
	{
		fclose(f);
	}
	return res;
}
