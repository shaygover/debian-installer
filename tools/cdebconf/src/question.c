/***********************************************************************
 *
 * cdebconf - An implementation of the Debian Configuration Management
 *            System
 *
 * File: question.c
 *
 * Description: interfaces for handling debconf questions
 *
 * $Id: question.c,v 1.21 2002/11/22 22:33:16 barbier Exp $
 *
 * cdebconf is (c) 2000-2001 Randolph Chung and others under the following
 * license.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ***********************************************************************/
#include "common.h"
#include "question.h"
#include "template.h"
#include "strutl.h"
#include "configuration.h"
#include "database.h"

struct question *question_new(const char *tag)
{
	struct question *q;

	q = NEW(struct question);
	memset(q, 0, sizeof(struct question));
	q->ref = 1;
	q->tag = STRDUP(tag);

	return q;
}

void question_delete(struct question *question)
{
    if (question->template)
        template_deref(question->template);
    DELETE(question);
}

void question_ref(struct question *q)
{
	++q->ref;
}

void question_deref(struct question *q)
{
	if (q == NULL) return;
	if (--q->ref == 0)
		question_delete(q);
}

/*
 * Function: question_dup
 * Input: q - the question to be duplicated
 * Output: a deep copy of the question struct passed as input.  the template 
 *         pointer is not changed
 * Description: duplicate a questions
 * Assumptions: all allocations succeed
 * Todo: 
 */

struct question *question_dup(struct question *q)
{
        struct question *ret = question_new(q->tag);
        struct questionvariable *qv = q->variables;
        struct questionowner *qo = q->owners;
        ret->value = STRDUP(q->value);
        ret->flags = q->flags;
        ret->template = q->template;
        template_ref(ret->template);
//        ret->template = template_dup(q->template);
        while (qv)
        {
                question_variable_add(ret,qv->variable,qv->value);
                qv = qv->next;
        }
        while (qo)
        {
                question_owner_add(ret,qo->owner);
                qo = qo->next;
        }
        return ret;
}

void question_setvalue(struct question *q, const char *value)
{
	/* Be careful about the self-assignment case... */
	if (q->value != value)
	{
		DELETE(q->value);
		q->value = STRDUP(value);
	}
}

const char *question_getvalue(struct question *q, const char *lang)
{
	if (q->value)
		return q->value;
	return q->template->lget(q->template, lang, "default");
}

void question_variable_add(struct question *q, const char *var, 	
	const char *value)
{
	struct questionvariable *qvi = q->variables;
	struct questionvariable **qlast = &q->variables;

	INFO(INFO_DEBUG, "Adding [%s] -> [%s]\n", var, value);
	for (; qvi != 0; qlast = &qvi->next, qvi = qvi->next)
		if (strcmp(qvi->variable, var) == 0 && qvi->value != value)
		{
			DELETE(qvi->value);
			qvi->value = STRDUP(value);
			return;
		}
	
	qvi = NEW(struct questionvariable);
	memset(qvi, 0, sizeof(struct questionvariable));
	qvi->variable = STRDUP(var);
	qvi->value = STRDUP(value);
	*qlast = qvi;
}

void question_owner_add(struct question *q, const char *owner)
{
	struct questionowner **ownerp = &q->owners;
	
	while (*ownerp != 0)
	{
		if (strcmp((*ownerp)->owner, owner) == 0)
			return;
		ownerp = &(*ownerp)->next;
	}

	*ownerp = NEW(struct questionowner);
	(*ownerp)->owner = STRDUP(owner);
	(*ownerp)->next = 0;
}

void question_owner_delete(struct question *q, const char *owner)
{
	struct questionowner **ownerp, *nextp;

	for (ownerp = &q->owners; *ownerp != 0; ownerp = &(*ownerp)->next)
	{
		if (strcmp((*ownerp)->owner, owner) == 0)
		{
			nextp = (*ownerp)->next;
			if (nextp == 0)
			{
				nextp = *ownerp;
				*ownerp = 0;
			}
			else
			{
				**ownerp = *nextp;
			}
			DELETE(nextp->owner);
			DELETE(nextp);
		}
	}
}

static int question_expand_vars(struct question *q, const char *field, 
	char *buf, size_t maxlen)
{
	int i = 0;
	const char *p = field, *varend;
	char var[100];
	struct questionvariable *qvi;

	memset(buf, 0, maxlen);
	if (p == 0) return 0;

	while (*p != 0 && i < maxlen - 1)
	{
		/* is this a variable string? */
		if (*p != '$' || *(p+1) != '{') 
		{
			buf[i++] = *p++;
			continue;
		}

		/* look for the end of the variable */
		varend = p + 2;
		while (*varend != 0 && *varend != '}') varend++;
		/* didn't find it? then don't consider this a variable */
		if (*varend == 0)
		{
			buf[i++] = *p++;
			continue;
		}

		strncpy(var, p+2, varend-(p+2));
		var[varend-(p+2)] = 0;

		for (qvi = q->variables; qvi != 0; qvi = qvi->next)
			if (strcmp(qvi->variable, var) == 0)
				break;

		if (qvi != 0)
		{
			strvacat(buf, maxlen, qvi->value, NULL);
			i = strlen(buf);
		}

		p = varend + 1;
	}
	return DC_OK;
}

const char *question_get_field(struct question *q, const char *lang,
	const char *field)
{
	static char buf[4096] = {0};
	if (strcmp(field, "value") == 0)
		question_expand_vars(q,
			question_getvalue(q, lang),
			buf, sizeof(buf));
        else
		question_expand_vars(q,
			q->template->lget(q->template, lang, field),
			buf, sizeof(buf));
	return buf;
}

