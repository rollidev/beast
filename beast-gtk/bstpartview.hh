// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#ifndef __BST_PART_VIEW_H__
#define __BST_PART_VIEW_H__

#include	"bstitemview.hh"

/* --- type macros --- */
#define BST_TYPE_PART_VIEW              (bst_part_view_get_type ())
#define BST_PART_VIEW(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), BST_TYPE_PART_VIEW, BstPartView))
#define BST_PART_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BST_TYPE_PART_VIEW, BstPartViewClass))
#define BST_IS_PART_VIEW(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), BST_TYPE_PART_VIEW))
#define BST_IS_PART_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BST_TYPE_PART_VIEW))
#define BST_PART_VIEW_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), BST_TYPE_PART_VIEW, BstPartViewClass))


/* --- structures & typedefs --- */
typedef	struct	_BstPartView		BstPartView;
typedef	struct	_BstPartViewClass	BstPartViewClass;
struct _BstPartView
{
  BstItemView	 parent_object;
};
struct _BstPartViewClass
{
  BstItemViewClass parent_class;
};


/* --- prototypes --- */
GType		bst_part_view_get_type	(void);
GtkWidget*      bst_part_view_new       (SfiProxy song);

#endif /* __BST_PART_VIEW_H__ */
