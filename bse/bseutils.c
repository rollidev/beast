/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1998-2002 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bseutils.h"

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "gslieee754.h"


/* --- file utils --- */
void
bse_str_slist_free (GSList *slist)
{
  GSList *node;

  for (node = slist; node; node = node->next)
    g_free (node->data);
  g_slist_free (slist);
}

static GSList*
slist_prepend_uniq (GSList  *slist,
		    gchar   *string,
		    gboolean free_str)
{
  GSList *node;

  for (node = slist; node; node = node->next)
    if (strcmp (node->data, string) == 0)
      {
	if (free_str)
	  g_free (string);
	return slist;
      }
  
  return g_slist_prepend (slist, string);
}

static GSList*
slist_prepend_uniq_dedup (GSList *slist,
			  gchar  *dupstr)
{
  return slist_prepend_uniq (slist, dupstr, TRUE);
}

/**
 * bse_search_path_list_matches
 * @search_path: colon (semicolon under win32) seperated search path with '?' and '*' wildcards
 * @cwd:         assumed current working directoy (to interpret './' in search_path)
 * @RETURNS:     a singly linked list with newly allocated strings
 * This function takes a search path with wildcards and lists all existing
 * directories matching components of the search path.
 */
GSList*
bse_search_path_list_matches (const gchar *search_path,
			      const gchar *cwd)
{
  GSList *entries, *slist, *node, *matches = NULL;
  gchar *free_me = NULL;

  g_return_val_if_fail (search_path != NULL, NULL);

  if (!cwd)
    {
      free_me = g_get_current_dir ();
      cwd = free_me;
    }

  entries = bse_search_path_list_entries (search_path);
  for (slist = entries; slist; slist = slist->next)
    {
      GSList *pdirs = bse_path_pattern_list_matches (slist->data, cwd, G_FILE_TEST_IS_DIR);

      for (node = pdirs; node; node = node->next)
	matches = slist_prepend_uniq_dedup (matches, node->data);
      g_slist_free (pdirs);
    }
  bse_str_slist_free (entries);
  g_free (free_me);

  return g_slist_reverse (matches);
}

/**
 * bse_search_path_list_files
 * @search_path:  colon (semicolon under win32) seperated search path with '?' and '*' wildcards
 * @file_pattern: wildcard pattern for file names
 * @cwd:          assumed current working directoy (to interpret './' in search_path)
 * @file_test:    GFileTest file test condition (e.g. G_FILE_TEST_IS_REGULAR) or 0
 * @RETURNS:      a singly linked list with newly allocated strings
 * Given a search path with wildcards, list all files matching @file_pattern,
 * contained in the directories which the search path matches. Files that do
 * not pass @file_test are not listed.
 */
GSList*
bse_search_path_list_files (const gchar *search_path,
			    const gchar *file_pattern,
			    const gchar *cwd,
			    GFileTest    file_test)
{
  GSList *search_dirs, *slist, *node, *files = NULL;
  GHashTable *fhash;
  gchar *free_me = NULL;
  
  g_return_val_if_fail (search_path != NULL, NULL);

  if (!file_pattern)
    file_pattern = "*";
  if (!cwd)
    {
      free_me = g_get_current_dir ();
      cwd = free_me;
    }

  fhash = g_hash_table_new (g_str_hash, g_str_equal);
  search_dirs = bse_search_path_list_matches (search_path, cwd);
  for (slist = search_dirs; slist; slist = slist->next)
    {
      gchar *pattern = g_strconcat (slist->data, G_DIR_SEPARATOR_S, file_pattern, NULL);
      GSList *pfiles = bse_path_pattern_list_matches (pattern, cwd, file_test);

      g_free (pattern);
      for (node = pfiles; node; node = node->next)
	{
	  gchar *file = node->data;

	  if (g_hash_table_lookup (fhash, file))
	    g_free (file);
	  else
	    {
	      g_hash_table_insert (fhash, file, file);
	      files = g_slist_prepend (files, file);
	    }
	}
      g_slist_free (pfiles);
    }
  bse_str_slist_free (search_dirs);
  g_hash_table_destroy (fhash);
  
  g_free (free_me);

  return g_slist_reverse (files);
}

/**
 * bse_search_path_list_entries
 * @search_path: colon (semicolon under win32) seperated search path
 * @RETURNS:     a singly linked list with newly allocated strings
 * This function takes a search path and returns a singly linked list
 * containing the seperated path entries.
 */
GSList*
bse_search_path_list_entries (const gchar *search_path)
{
  const gchar *p, *sep;
  GSList *slist = NULL;
  
  g_return_val_if_fail (search_path != NULL, NULL);

  p = search_path;
  sep = strchr (p, G_SEARCHPATH_SEPARATOR);
  while (sep)
    {
      if (sep > p)
	slist = slist_prepend_uniq_dedup (slist, g_strndup (p, sep - p));
      p = sep + 1;
      sep = strchr (p, G_SEARCHPATH_SEPARATOR);
    }
  if (*p)
    slist = slist_prepend_uniq_dedup (slist, g_strdup (p));
  return g_slist_reverse (slist);
}

static GSList*
hashslist_prepend_uniq_dedup (GSList     *slist,
			      GHashTable *listhash,
			      gchar      *string)
{
  if (g_hash_table_lookup (listhash, string))
    g_free (string);
  else
    {
      g_hash_table_insert (listhash, string, string);
      slist = g_slist_prepend (slist, string);
    }
  return slist;
}

static GSList*
prepend_dir_entries (GSList     *slist,
		     GHashTable *listhash,
		     gchar      *dir,
		     gchar      *file,
		     GFileTest   file_test)
{
  g_return_val_if_fail (dir && file, NULL);

  if (strchr (file, '?') || strchr (file, '*'))
    {
      GPatternSpec *pspec = g_pattern_spec_new (file);
      gchar *dirpath = g_strconcat (dir, G_DIR_SEPARATOR_S, NULL);
      DIR *dd = opendir (dirpath);

      g_free (dirpath);
      if (dd)
	{
	  struct dirent *d_entry = readdir (dd);

	  while (d_entry)
	    {
	      if (!(d_entry->d_name[0] == '.' && d_entry->d_name[1] == 0) &&
		  !(d_entry->d_name[0] == '.' && d_entry->d_name[1] == '.' && d_entry->d_name[2] == 0) &&
		  g_pattern_match_string (pspec, d_entry->d_name))
		{
		  gchar *str = g_strconcat (dir, G_DIR_SEPARATOR_S, d_entry->d_name, NULL);

		  if (file_test && !g_file_test (str, file_test))
		    g_free (str);
		  else
		    slist = hashslist_prepend_uniq_dedup (slist, listhash, str);
		}
	      d_entry = readdir (dd);
	    }
	  closedir (dd);
	}
      g_pattern_spec_free (pspec);
    }
  else if (strcmp (file, ".") == 0)
    slist = hashslist_prepend_uniq_dedup (slist, listhash, g_strdup (dir));
  else if (strcmp (file, "..") == 0)
    {
      gchar *s = strrchr (dir, G_DIR_SEPARATOR);

      if (s)
	slist = hashslist_prepend_uniq_dedup (slist, listhash, g_strndup (dir, s - dir));
    }
  else
    {
      gchar *s = g_strconcat (dir, G_DIR_SEPARATOR_S, file, NULL);

      if (!file_test)
	file_test = G_FILE_TEST_EXISTS;
      if (!g_file_test (s, file_test))
	g_free (s);
      else
	slist = hashslist_prepend_uniq_dedup (slist, listhash, s);
    }
  return slist;
}

static GSList*
match_abs_path (gchar    *p,
		GFileTest file_test)
{
  GSList *slist, *node, *new_slist = NULL;
  GHashTable *strhash = g_hash_table_new (g_str_hash, g_str_equal);
  gchar *sep;
  
  g_return_val_if_fail (p != NULL, NULL);

  sep = strchr (p, G_DIR_SEPARATOR);
  g_return_val_if_fail (sep != NULL, NULL);

  *sep++ = 0;
  while (*sep == G_DIR_SEPARATOR)
    *sep++ = 0;

  /* get root into list */
  slist = g_slist_prepend (NULL, g_strdup (p));

  p = sep;
  sep = strchr (p, G_DIR_SEPARATOR);
  while (sep)
    {
      *sep++ = 0;
      while (*sep == G_DIR_SEPARATOR)
	*sep++ = 0;
      for (node = slist; node; node = node->next)
	new_slist = prepend_dir_entries (new_slist, strhash, node->data, p, G_FILE_TEST_IS_DIR);
      bse_str_slist_free (slist);
      slist = new_slist;
      g_hash_table_destroy (strhash);
      strhash = g_hash_table_new (g_str_hash, g_str_equal);
      if (!slist)
	return NULL;
      new_slist = NULL;
      p = sep;
      sep = strchr (p, G_DIR_SEPARATOR);
    }
  for (node = slist; node; node = node->next)
    new_slist = prepend_dir_entries (new_slist, strhash, node->data, p, file_test);
  bse_str_slist_free (slist);
  slist = new_slist;
  g_hash_table_destroy (strhash);
  
  return slist;
}

/**
 * bse_path_pattern_list_matches
 * @file_pattern: a filename spanning directories containing wildcards '?' or '*'
 * @cwd:          assumed current working directoy (to interpret relative file_pattern)
 * @file_test:	  GFileTest file test condition (e.g. G_FILE_TEST_IS_REGULAR) or 0
 * @RETURNS:      a singly linked list with newly allocated strings
 * This function takes filename with wildcards, transforms it into an absolute
 * pathname using @cwd if necessary and lists all files matching the resulting
 * file name pattern and passing @file_test.
 */
GSList*
bse_path_pattern_list_matches (const gchar *file_pattern,
			       const gchar *cwd,
			       GFileTest    file_test)
{
  gchar *tmp, *free_me = NULL, cwdbuf[16];
  GSList *slist;

  g_return_val_if_fail (file_pattern != NULL, NULL);

  if (!cwd)
    {
      free_me = g_get_current_dir ();
      cwd = free_me;
    }
  if (cwd[0] != G_DIR_SEPARATOR)	/* FIXME: breaks on windows */
    {
      cwdbuf[0] = G_DIR_SEPARATOR;
      cwdbuf[1] = 0;
      cwd = cwdbuf;
    }

  if (file_pattern[0] == G_DIR_SEPARATOR)	/* FIXME: breaks on windows */
    tmp = g_strdup (file_pattern);
  else
    tmp = g_strconcat (cwd, G_DIR_SEPARATOR_S, file_pattern, NULL);
  g_free (free_me);
  slist = match_abs_path (tmp, file_test);
  g_free (tmp);
  return slist;
}


/* --- record utils --- */
BseNoteDescription*
bse_note_description (SfiInt note,
                      gint   fine_tune)
{
  BseNoteDescription *info = bse_note_description_new ();

  if (note >= BSE_MIN_NOTE && note <= BSE_MAX_NOTE)
    {
      gchar letter;
      info->note = note;
      bse_note_examine (info->note,
                        &info->octave,
                        &info->semitone,
                        &info->upshift,
                        &letter);
      info->letter = letter;
      info->fine_tune = CLAMP (fine_tune, BSE_MIN_FINE_TUNE, BSE_MAX_FINE_TUNE);
      info->freq = bse_note_to_tuned_freq (info->note, info->fine_tune);
      info->name = bse_note_to_string (info->note);
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  else
    {
      info->note = BSE_NOTE_VOID;
      info->name = NULL;
      info->max_fine_tune = BSE_MAX_FINE_TUNE;
      info->kammer_note = BSE_KAMMER_NOTE;
    }
  return info;
}

BsePartNote*
bse_part_note (guint    id,
	       guint    tick,
	       guint    duration,
	       gint     note,
	       gint     fine_tune,
	       gfloat   velocity,
	       gboolean selected)
{
  BsePartNote *pnote = bse_part_note_new ();

  pnote->id = id;
  pnote->tick = tick;
  pnote->duration = duration;
  pnote->note = note;
  pnote->fine_tune = fine_tune;
  pnote->velocity = velocity;
  pnote->selected = selected != FALSE;

  return pnote;
}

void
bse_part_note_seq_take_append (BsePartNoteSeq *seq,
			       BsePartNote    *element)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (element != NULL);

  bse_part_note_seq_append (seq, element);
  bse_part_note_free (element);
}

BsePartControl*
bse_part_control (guint              id,
                  guint              tick,
                  BseMidiSignalType  ctype,
                  gfloat             value,
                  gboolean           selected)
{
  BsePartControl *pctrl = bse_part_control_new ();

  pctrl->id = id;
  pctrl->tick = tick;
  pctrl->control_type = ctype;
  pctrl->value = value;
  pctrl->selected = selected != FALSE;

  return pctrl;
}

void
bse_part_control_seq_take_append (BsePartControlSeq *seq,
                                  BsePartControl    *element)
{
  g_return_if_fail (seq != NULL);
  g_return_if_fail (element != NULL);

  bse_part_control_seq_append (seq, element);
  bse_part_control_free (element);
}

void
bse_note_sequence_resize (BseNoteSequence *rec,
			  guint            length)
{
  guint fill = rec->notes->n_notes;

  bse_note_seq_resize (rec->notes, length);
  while (fill < length)
    rec->notes->notes[fill++] = SFI_KAMMER_NOTE;
}

guint
bse_note_sequence_length (BseNoteSequence *rec)
{
  return rec->notes->n_notes;
}


/* --- balance calculation --- */
double
bse_balance_get (double level1,
                 double level2)
{
  return level2 - level1;
}

void
bse_balance_set (double balance,
                 double *level1,
                 double *level2)
{
  double l1 = *level1, l2 = *level2;
  double d = (l1 + l2) * 0.5;
  l1 = d - balance * 0.5;
  l2 = d + balance * 0.5;
  if (l1 < 0)
    {
      l2 += -l1;
      l1 = 0;
    }
  if (l1 > 100)
    {
      l2 -= l1 - 100;
      l1 = 100;
    }
  if (l2 < 0)
    {
      l1 += -l2;
      l2 = 0;
    }
  if (l2 > 100)
    {
      l1 -= l2 - 100;
      l2 = 100;
    }
  *level1 = l1;
  *level2 = l2;
}


/* --- icons --- */
BseIcon*
bse_icon_from_pixdata (const BsePixdata *pixdata)
{
  BseIcon *icon;
  guint bpp, encoding;

  g_return_val_if_fail (pixdata != NULL, NULL);

  if (pixdata->width < 1 || pixdata->width > 128 ||
      pixdata->height < 1 || pixdata->height > 128)
    {
      g_warning (G_GNUC_PRETTY_FUNCTION "(): `pixdata' exceeds dimension limits (%ux%u)",
		 pixdata->width, pixdata->height);
      return NULL;
    }
  bpp = pixdata->type & BSE_PIXDATA_RGB_MASK;
  encoding = pixdata->type & BSE_PIXDATA_ENCODING_MASK;
  if ((bpp != BSE_PIXDATA_RGB && bpp != BSE_PIXDATA_RGBA) ||
      (encoding && encoding != BSE_PIXDATA_1BYTE_RLE))
    {
      g_warning (G_GNUC_PRETTY_FUNCTION "(): `pixdata' format/encoding unrecognized");
      return NULL;
    }
  if (!pixdata->encoded_pix_data)
    return NULL;

  icon = bse_icon_new ();
  icon->bytes_per_pixel = bpp;
  icon->width = pixdata->width;
  icon->height = pixdata->height;
  sfi_bblock_resize (icon->pixels, icon->width * icon->height * icon->bytes_per_pixel);

  if (encoding == BSE_PIXDATA_1BYTE_RLE)
    {
      const guint8 *rle_buffer = pixdata->encoded_pix_data;
      guint8 *image_buffer = icon->pixels->bytes;
      guint8 *image_limit = image_buffer + icon->width * icon->height * bpp;
      gboolean check_overrun = FALSE;
      
      while (image_buffer < image_limit)
	{
	  guint length = *(rle_buffer++);
	  
	  if (length & 128)
	    {
	      length = length - 128;
	      check_overrun = image_buffer + length * bpp > image_limit;
	      if (check_overrun)
		length = (image_limit - image_buffer) / bpp;
	      if (bpp < 4)
		do
		  {
		    memcpy (image_buffer, rle_buffer, 3);
		    image_buffer += 3;
		  }
		while (--length);
	      else
		do
		  {
		    memcpy (image_buffer, rle_buffer, 4);
		    image_buffer += 4;
		  }
		while (--length);
	      rle_buffer += bpp;
	    }
	  else
	    {
	      length *= bpp;
	      check_overrun = image_buffer + length > image_limit;
	      if (check_overrun)
		length = image_limit - image_buffer;
	      memcpy (image_buffer, rle_buffer, length);
	      image_buffer += length;
	      rle_buffer += length;
	    }
	}
      if (check_overrun)
	g_warning (G_GNUC_PRETTY_FUNCTION "(): `pixdata' encoding screwed");
    }
  else
    memcpy (icon->pixels->bytes, pixdata->encoded_pix_data, icon->width * icon->height * bpp);
  
  return icon;
}

static inline const guint8 *
get_uint32 (const guint8 *stream, guint *result)
{
  *result = (stream[0] << 24) + (stream[1] << 16) + (stream[2] << 8) + stream[3];
  return stream + 4;
}

BseIcon*
bse_icon_from_pixstream (const guint8 *pixstream)
{
  BsePixdata pixd;
  const guint8 *s = pixstream;
  guint len, type, rowstride, width, height;

  g_return_val_if_fail (pixstream != NULL, NULL);

  if (strncmp (s, "GdkP", 4) != 0)
    return NULL;
  s += 4;

  s = get_uint32 (s, &len);
  if (len < 24)
    return NULL;

  s = get_uint32 (s, &type);
  if (type != 0x02010002 &&     /* RLE/8bit/RGBA */
      type != 0x01010002)       /* RAW/8bit/RGBA */
    return NULL;

  s = get_uint32 (s, &rowstride);
  s = get_uint32 (s, &width);
  s = get_uint32 (s, &height);
  if (width < 1 || height < 1)
    return NULL;

  pixd.type = BSE_PIXDATA_RGBA | (type >> 24 == 2 ? BSE_PIXDATA_1BYTE_RLE : 0);
  pixd.width = width;
  pixd.height = height;
  pixd.encoded_pix_data = s;
  return bse_icon_from_pixdata (&pixd);
}


/* --- ID allocator --- */
#define	ID_WITHHOLD_BUFFER_SIZE		59
static gulong  id_counter = 1;
static gulong  n_buffer_ids = 0;
static gulong  id_buffer[ID_WITHHOLD_BUFFER_SIZE];
static gulong  id_buffer_pos = 0;
static gulong  n_free_ids = 0;
static gulong *free_id_buffer = NULL;

void
bse_id_free (gulong id)
{
  g_return_if_fail (id > 0);

  /* release oldest withheld id */
  if (n_buffer_ids >= ID_WITHHOLD_BUFFER_SIZE)
    {
      gulong n = n_free_ids++;
      gulong size = sfi_alloc_upper_power2 (n_free_ids);
      if (size != sfi_alloc_upper_power2 (n))
	free_id_buffer = g_renew (gulong, free_id_buffer, size);
      free_id_buffer[n] = id_buffer[id_buffer_pos];
    }

  /* release id */
  id_buffer[id_buffer_pos++] = id;
  n_buffer_ids = MAX (n_buffer_ids, id_buffer_pos);
  if (id_buffer_pos >= ID_WITHHOLD_BUFFER_SIZE)
    id_buffer_pos = 0;
}

gulong
bse_id_alloc (void)
{
  if (n_free_ids)
    {
      gulong random_pos = (id_counter + id_buffer[id_buffer_pos]) % n_free_ids--;
      gulong id = free_id_buffer[random_pos];
      free_id_buffer[random_pos] = free_id_buffer[n_free_ids];
      return id;
    }
  return id_counter++;
}


/* --- miscellaeous --- */
guint
bse_string_hash (gconstpointer string)
{
  const gchar *p = string;
  guint h = 0;
  if (!p)
    return 1;
  for (; *p; p++)
    h = (h << 5) - h + *p;
  return h;
}

gint
bse_string_equals (gconstpointer string1,
		   gconstpointer string2)
{
  if (string1 && string2)
    return strcmp (string1, string2) == 0;
  else
    return string1 == string2;
}

void
bse_bbuffer_puts (gchar        bbuffer[BSE_BBUFFER_SIZE],
		  const gchar *string)
{
  g_return_if_fail (bbuffer != NULL);
  
  strncpy (bbuffer, string, BSE_BBUFFER_SIZE - 1);
  bbuffer[BSE_BBUFFER_SIZE - 1] = 0;
}

guint
bse_bbuffer_printf (gchar        bbuffer[BSE_BBUFFER_SIZE],
		    const gchar *format,
		    ...)
{
  va_list args;
  guint l;

  g_return_val_if_fail (bbuffer != NULL, 0);
  g_return_val_if_fail (format != NULL, 0);

  va_start (args, format);
  l = g_vsnprintf (bbuffer, BSE_BBUFFER_SIZE, format, args);
  va_end (args);

  return l;
}
