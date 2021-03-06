// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_TRACK_SYNTH_DIALOG_H__
#define __BST_TRACK_SYNTH_DIALOG_H__

#include "bstutils.hh"
#include "bstwaveview.hh"
#include "bstsoundfontview.hh"


/* --- Gtk+ type macros --- */
#define BST_TYPE_TRACK_SYNTH_DIALOG            (bst_track_synth_dialog_get_type ())
#define BST_TRACK_SYNTH_DIALOG(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_TRACK_SYNTH_DIALOG, BstTrackSynthDialog))
#define BST_TRACK_SYNTH_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_TRACK_SYNTH_DIALOG, BstTrackSynthDialogClass))
#define BST_IS_TRACK_SYNTH_DIALOG(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_TRACK_SYNTH_DIALOG))
#define BST_IS_TRACK_SYNTH_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_TRACK_SYNTH_DIALOG))
#define BST_TRACK_SYNTH_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BST_TYPE_TRACK_SYNTH_DIALOG, BstTrackSynthDialogClass))


/* --- structures & typedefs --- */
typedef struct _BstTrackSynthDialog      BstTrackSynthDialog;
typedef struct _BstTrackSynthDialogClass BstTrackSynthDialogClass;
typedef void (*BstTrackSynthDialogSelected)     (gpointer                data,
                                                 SfiProxy                proxy,
                                                 BstTrackSynthDialog    *tsdialog);
struct _BstTrackSynthDialog
{
  GxkDialog      parent_instance;
  GtkNotebook   *notebook;
  GtkWidget     *wpage;         /* wave repo item view */
  GtkWidget     *spage;         /* synth list */
  GtkWidget     *sfont_page;    /* sound font patch selection */
  GtkWidget     *ok;            /* ok button */
  GtkWindow     *parent_window;
  guint          ignore_activate : 1;
  GtkTreeModel  *pstore;        /* proxy store */
  GtkTreeView   *tview;         /* synth selection tree view */
  BstTrackSynthDialogSelected  selected_callback;
  gpointer                     selected_data;
  GxkFreeFunc                  selected_cleanup;
};
struct _BstTrackSynthDialogClass
{
  GxkDialogClass parent_class;
};


/* --- prototypes --- */
GType      bst_track_synth_dialog_get_type (void);
GtkWidget* bst_track_synth_dialog_popup    (gpointer                     parent_widget,
                                            SfiProxy                     track,
                                            const gchar                 *candidate_label,
                                            const gchar                 *candidate_tooltip,
                                            BseIt3mSeq                  *candidates,
                                            const gchar                 *wrepo_label,
                                            const gchar                 *wrepo_tooltip,
                                            SfiProxy                     wrepo,
                                            const gchar                 *sfrepo_label,
                                            const gchar                 *sfrepo_tooltip,
                                            SfiProxy                     sfrepo,
                                            BstTrackSynthDialogSelected  selected_callback,
                                            gpointer                     selected_data,
                                            GxkFreeFunc                  selected_cleanup);
void       bst_track_synth_dialog_set      (BstTrackSynthDialog         *self,
                                            BseIt3mSeq                  *iseq,
                                            SfiProxy                     wrepo,
					    SfiProxy			 sfrepo);


#endif /* __BST_TRACK_SYNTH_DIALOG_H__ */
