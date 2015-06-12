// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html
#include "bstutils.hh"
#include "bstapp.hh"
#include "bstsplash.hh"
#include "bstxkb.hh"
#include "bstgconfig.hh"
#include "bstkeybindings.hh"
#include "bstskinconfig.hh"
#include "bstmsgabsorb.hh"
#include "bstusermessage.hh"
#include "bstparam.hh"
#include "bstpreferences.hh"
#include "topconfig.h"
#include "data/beast-images.h"
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>

extern "C" void bse_object_debug_leaks (void); // FIXME

/* --- prototypes --- */
static void			bst_early_parse_args	(int *argc_p, char **argv);
static void			bst_print_blurb		(void);
static void			bst_exit_print_version	(void);
static void                     bst_init_aida_idl       ();

/* --- variables --- */
gboolean            bst_developer_hints = FALSE;
gboolean            bst_debug_extensions = FALSE;
gboolean            bst_main_loop_running = TRUE;
static GtkWidget   *beast_splash = NULL;
static gboolean     registration_done = FALSE;
static gboolean     arg_force_xkb = FALSE;
static gboolean     register_core_plugins = TRUE;
static gboolean     register_ladspa_plugins = TRUE;
static gboolean     register_scripts = TRUE;
static gboolean     may_auto_update_bse_rc_file = TRUE;
static bool         rewrite_bse_file = false;

/* --- functions --- */
static void
server_registration (SfiProxy     server,
		     SfiChoice    rchoice,
		     const gchar *what,
		     const gchar *error,
		     gpointer     data)
{
  BseRegistrationType rtype = bse_registration_type_from_choice (rchoice);

  if (rtype == BSE_REGISTER_DONE)
    registration_done = TRUE;
  else
    {
      const char *base = strrchr (what, '/');
      bst_splash_update_item ((GtkWidget*) data, base ? base + 1 : what);
      if (error && error[0])
	g_message ("failed to register \"%s\": %s", what, error);
    }
}

int
main (int   argc,
      char *argv[])
{
  GdkPixbufAnimation *anim;
  gchar *string;
  GSource *source;
  /* initialize i18n */
  bindtextdomain (BST_GETTEXT_DOMAIN, BST_PATH_LOCALE);
  bind_textdomain_codeset (BST_GETTEXT_DOMAIN, "UTF-8");
  textdomain (BST_GETTEXT_DOMAIN);
  setlocale (LC_ALL, "");
  /* initialize random numbers */
  struct timeval tv;
  gettimeofday (&tv, NULL);
  srand48 (tv.tv_usec + (tv.tv_sec << 16));
  srand (tv.tv_usec + (tv.tv_sec << 16));

  // initialize threading and GLib types
  Rapicorn::ThreadInfo::self().name ("Beast GUI");
  Bse::TaskRegistry::add (Rapicorn::ThreadInfo::self().name(), Rapicorn::ThisThread::process_pid(), Rapicorn::ThisThread::thread_pid());

  g_type_init ();
  /* initialize Birnet/Sfi */
  sfi_init (&argc, argv, "BEAST");
  /* ensure SFI can wake us up */
  /* initialize Gtk+ and go into threading mode */
  bst_early_parse_args (&argc, argv);
  gtk_init (&argc, &argv);
  GDK_THREADS_ENTER ();
  /* initialize Gtk+ Extension Kit */
  gxk_init ();
  /* documentation search paths */
  gxk_text_add_tsm_path (BST_PATH_DOCS);
  gxk_text_add_tsm_path (BST_PATH_IMAGES);
  gxk_text_add_tsm_path (".");
  /* now, we can popup the splash screen */
  beast_splash = bst_splash_new ("BEAST-Splash", BST_SPLASH_WIDTH, BST_SPLASH_HEIGHT, 15);
  bst_splash_set_title (beast_splash, _("BEAST Startup"));
  gtk_object_set_user_data (GTK_OBJECT (beast_splash), NULL);	/* fix for broken user_data in 2.2 */
  bst_splash_set_text (beast_splash,
		       Rapicorn::string_format ("<b><big>BEAST</big></b>\n"
                                                "<b>The Better Audio System</b>\n"
                                                "<b>Version %s (%s)</b>\n",
                                                BST_VERSION, BST_VERSION_HINT));
  bst_splash_update_entity (beast_splash, _("Startup"));
  bst_splash_show_grab (beast_splash);

  /* BEAST initialization */
  bst_splash_update_item (beast_splash, _("Initializers"));
  _bst_init_utils ();
  _bst_init_params ();
  _bst_gconfig_init ();
  _bst_skin_config_init ();
  _bst_msg_absorb_config_init ();

  /* parse rc file */
  bst_splash_update_item (beast_splash, _("RC Files"));
  bst_preferences_load_rc_files();

  /* show splash images */
  bst_splash_update_item (beast_splash, _("Splash Image"));
  string = g_strconcat (BST_PATH_IMAGES, G_DIR_SEPARATOR_S, BST_SPLASH_IMAGE, NULL);
  anim = gdk_pixbuf_animation_new_from_file (string, NULL);
  g_free (string);
  bst_splash_update ();
  if (anim)
    {
      bst_splash_set_animation (beast_splash, anim);
      g_object_unref (anim);
    }

  /* start BSE core and connect */
  bst_splash_update_item (beast_splash, _("BSE Core"));
  Bse::String bseoptions = Bse::string_format ("debug-extensions=%d", bst_debug_extensions);
  Bse::init_async (&argc, argv, "BEAST", Bse::string_split (bseoptions, ":"));
  sfi_glue_context_push (Bse::init_glue_context ("BEAST", bst_main_loop_wakeup));
  source = g_source_simple (GDK_PRIORITY_EVENTS, // G_PRIORITY_HIGH - 100,
			    (GSourcePending) sfi_glue_context_pending,
			    (GSourceDispatch) sfi_glue_context_dispatch,
			    NULL, NULL, NULL);
  g_source_attach (source, NULL);
  g_source_unref (source);

  /* now that the BSE thread runs, drop scheduling priorities if we have any */
  setpriority (PRIO_PROCESS, getpid(), 0);

  /* watch registration notifications on server */
  bse_proxy_connect (BSE_SERVER,
		     "signal::registration", server_registration, beast_splash,
		     NULL);

  /* register core plugins */
  if (register_core_plugins)
    {
      bst_splash_update_entity (beast_splash, _("Plugins"));

      /* plugin registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_core_plugins (BSE_SERVER);
      while (!registration_done)
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	  sfi_glue_gc_run ();
	}
    }

  /* register LADSPA plugins */
  if (register_ladspa_plugins)
    {
      bst_splash_update_entity (beast_splash, _("LADSPA Plugins"));

      /* plugin registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_ladspa_plugins (BSE_SERVER);
      while (!registration_done)
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	  sfi_glue_gc_run ();
	}
    }

  /* debugging hook */
  const char *estring = g_getenv ("BEAST_SLEEP4GDB");
  if (estring && atoi (estring) > 0)
    {
      bst_splash_update_entity (beast_splash, "Debugging Hook");
      g_message ("going into sleep mode due to debugging request (pid=%u)", getpid ());
      g_usleep (2147483647);
    }

  /* register BSE scripts */
  if (register_scripts)
    {
      bst_splash_update_entity (beast_splash, _("Scripts"));

      /* script registration, this is done asyncronously,
       * so we wait until all are done
       */
      registration_done = FALSE;
      bse_server_register_scripts (BSE_SERVER);
      while (!registration_done)
	{
	  GDK_THREADS_LEAVE ();
	  g_main_iteration (TRUE);
	  GDK_THREADS_ENTER ();
	  sfi_glue_gc_run ();
	}
    }
  /* listen to BseServer notification */
  bst_splash_update_entity (beast_splash, _("Dialogs"));

  // hook up Bse aida IDL with main loop
  bst_init_aida_idl();

  bst_message_connect_to_server ();
  _bst_init_radgets ();
  /* open files given on command line */
  if (argc > 1)
    bst_splash_update_entity (beast_splash, _("Loading..."));
  BstApp *app = NULL;
  gboolean merge_with_last = FALSE;
  for (int i = 1; i < argc; i++)
    {
      bst_splash_update ();

      /* parse non-file args */
      if (strcmp (argv[i], "--merge") == 0)
        {
          merge_with_last = TRUE;
          continue;
        }

      /* load waves into the last project */
      if (bse_server_can_load (BSE_SERVER, argv[i]))
	{
	  if (app)
	    {
	      SfiProxy wrepo = bse_project_get_wave_repo (app->project);
	      gxk_status_printf (GXK_STATUS_WAIT, NULL, _("Loading \"%s\""), argv[i]);
	      BseErrorType error = bse_wave_repo_load_file (wrepo, argv[i]);
              bst_status_eprintf (error, _("Loading \"%s\""), argv[i]);
              if (error)
                sfi_error (_("Failed to load wave file \"%s\": %s"), argv[i], bse_error_blurb (error));
	    }
          else
	    {
	      SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");
	      SfiProxy wrepo = bse_project_get_wave_repo (project);
	      BseErrorType error = bse_wave_repo_load_file (wrepo, argv[i]);
	      if (!error)
		{
		  app = bst_app_new (project);
		  gxk_idle_show_widget (GTK_WIDGET (app));
		  bse_item_unuse (project);
		  gtk_widget_hide (beast_splash);
		}
              else
                {
                  bse_item_unuse (project);
                  sfi_error (_("Failed to load wave file \"%s\": %s"), argv[i], bse_error_blurb (error));
                }
	    }
          continue;
	}
      // load/merge projects
      if (!app || !merge_with_last)
        {
          SfiProxy project = bse_server_use_new_project (BSE_SERVER, argv[i]);
          BseErrorType error = bst_project_restore_from_file (project, argv[i], TRUE, TRUE);
          if (rewrite_bse_file)
            {
              Rapicorn::printerr ("%s: loading: %s\n", argv[i], bse_error_blurb (error));
              if (error)
                exit (1);
              if (unlink (argv[i]) < 0)
                {
                  perror (Rapicorn::string_format ("%s: failed to remove", argv[i]).c_str());
                  exit (2);
                }
              error = bse_project_store_bse (project, 0, argv[i], TRUE);
              Rapicorn::printerr ("%s: writing: %s\n", argv[i], bse_error_blurb (error));
              if (error)
                exit (3);
              exit (0);
            }
          if (!error || error == BSE_ERROR_FILE_NOT_FOUND)
            {
              error = BseErrorType (0);
              app = bst_app_new (project);
              gxk_idle_show_widget (GTK_WIDGET (app));
              gtk_widget_hide (beast_splash);
            }
          bse_item_unuse (project);
          if (error)
            sfi_error (_("Failed to load project \"%s\": %s"), argv[i], bse_error_blurb (error));
        }
      else
        {
          BseErrorType error = bst_project_restore_from_file (app->project, argv[i], TRUE, FALSE);
          if (error)
            sfi_error (_("Failed to merge project \"%s\": %s"), argv[i], bse_error_blurb (error));
        }
    }

  /* open default app window
   */
  if (!app)
    {
      SfiProxy project = bse_server_use_new_project (BSE_SERVER, "Untitled.bse");

      bse_project_get_wave_repo (project);
      app = bst_app_new (project);
      bse_item_unuse (project);
      gxk_idle_show_widget (GTK_WIDGET (app));
      gtk_widget_hide (beast_splash);
    }
  /* splash screen is definitely hidden here (still grabbing) */

  /* fire up release notes dialog
   */
  gboolean update_rc_files = FALSE;
  if (!BST_RC_VERSION || strcmp (BST_RC_VERSION, BST_VERSION))
    {
      const char *release_notes_title =
        "BEAST/BSE Release Notes";
      const char *release_notes_contents =
        "<tag-span-markup>"
        "<tagdef name=\"title\" weight=\"bold\" scale=\"1.5\"/>"
        "<tagdef name=\"hyperlink\" underline=\"single\" foreground=\"#0000ff\"/>"
        "<tagdef name=\"mono\" family=\"monospace\"/>"
        "<span tag=\"title\">BEAST/BSE " BST_VERSION " Release Notes</span>"
        "<newline/><newline/>"
        "This development series of Beast focusses on improving interoperability and feature integration. "
        "<newline/><newline/>"
        "Feedback is very much appreciated, please take the opportunity and provide your comments "
        "and questions in online forums like the Beast "
        "<span tag=\"hyperlink\"><xlink ref=\"http://bugzilla.gnome.org/buglist.cgi?query=product:beast\">Bugzilla</xlink></span> "
        "or the "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.testbit.eu/Beast_Contact\">mailing list</xlink></span>, "
        "all of which can be reached through the links provided by the Help/ menu."
        "<newline/><newline/>"
        "The Beast website provides much more information like detailed "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.testbit.eu/Beast_News\">project news</xlink></span>, "
        "synthesis hints, documentation and a "
        "<span tag=\"hyperlink\"><xlink ref=\"http://beast.testbit.eu/Beast_Sound_Gallery\">sound archive</xlink></span>. "
        "<newline/>"
        "So either activate 'Help/Beast Website...' or go to:"
        "<newline/>"
        "<span tag=\"mono\">        </span><span tag=\"hyperlink\"><xlink ref=\"http://beast.testbit.eu/\">http://beast.testbit.eu</xlink></span>"
        "</tag-span-markup>";
      GtkWidget *sctext = gxk_scroll_text_create (GXK_SCROLL_TEXT_WRAP | GXK_SCROLL_TEXT_SANS | GXK_SCROLL_TEXT_CENTER, NULL);
      gxk_scroll_text_set_tsm (sctext, release_notes_contents);
      GtkWidget *rndialog = (GtkWidget*) gxk_dialog_new (NULL, NULL, GXK_DIALOG_DELETE_BUTTON, release_notes_title, sctext);
      gxk_dialog_set_sizes (GXK_DIALOG (rndialog), 320, 200, 540, 420);
      gxk_scroll_text_rewind (sctext);
      gxk_idle_show_widget (rndialog);
      update_rc_files = TRUE;
      bst_gconfig_set_rc_version (BST_VERSION);
    }

  /* release splash grab */
  gtk_widget_hide (beast_splash);
  bst_splash_release_grab (beast_splash);

  /* away into the main loop */
  while (bst_main_loop_running)
    {
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
      g_main_iteration (TRUE);
      GDK_THREADS_ENTER ();
    }
  bst_message_dialogs_popdown ();
  /* perform necessary cleanup cycles */
  GDK_THREADS_LEAVE ();
  while (g_main_iteration (FALSE))
    {
      GDK_THREADS_ENTER ();
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
    }
  GDK_THREADS_ENTER ();

  /* save BSE configuration */
  if (update_rc_files && !bst_preferences_saved())
    {
      if (may_auto_update_bse_rc_file)
        bse_server_save_preferences (BSE_SERVER);
      /* save BEAST configuration and accelerator map */
      gchar *file_name = BST_STRDUP_RC_FILE ();
      BseErrorType error = bst_rc_dump (file_name);
      if (error)
	g_warning ("failed to save rc-file \"%s\": %s", file_name, bse_error_blurb (error));
      g_free (file_name);
    }

  // perform necessary cleanup cycles
  GDK_THREADS_LEAVE ();
  while (g_main_iteration (FALSE))
    {
      GDK_THREADS_ENTER ();
      sfi_glue_gc_run ();
      GDK_THREADS_LEAVE ();
    }

  // misc cleanups
  bse_object_debug_leaks ();
  Bse::TaskRegistry::remove (Rapicorn::ThisThread::thread_pid());

  return 0;
}

/// wake up the main context used by the Beast main event looop.
void
bst_main_loop_wakeup ()
{
  g_main_context_wakeup (g_main_context_default ());
}

static void
echo_test_handler (const std::string &msg)
{
  g_print ("BST-Thread: got signal with message: %s\n", msg.c_str());
}

static void
bst_init_aida_idl()
{
  assert (bse_server == NULL);
  // connect to BSE thread and fetch server handle
  bse_server = Rapicorn::Aida::ObjectBroker::connect<Bse::ServerH> ("inproc://BSE-" BST_VERSION);
  assert (bse_server != NULL);
  assert (bse_server.proxy_id() == BSE_SERVER);
  assert (bse_server.from_proxy (BSE_SERVER) == bse_server);
  // hook Aida connection into our main loop
  Bse::AidaGlibSource *source = Bse::AidaGlibSource::create (Bse::ServerH::__aida_connection__());
  g_source_set_priority (source, G_PRIORITY_DEFAULT);
  g_source_attach (source, g_main_context_default());

  // perform Bse Aida tests
  if (0)
    {
      Rapicorn::printerr ("bse_server: %s\n", bse_server.debug_name());
      Bse::TestObjectH test = bse_server.get_test_object();
      test.sig_echo_reply() += echo_test_handler;
      const int test_result = test.echo_test ("foo");
      g_assert (test_result == 3);
    }
}

static void
bst_early_parse_args (int *argc_p, char **argv)
{
  uint argc = *argc_p;
  uint i, e;
  bool initialize_bse_and_exit = false;
  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "--") == 0)
        {
          argv[i] = NULL;
          break;
        }
      else if (strncmp (argv[i], "-:", 2) == 0)
	{
	  const gchar *flags = argv[i] + 2;
	  g_printerr ("BEAST(%s): pid = %u\n", BST_VERSION, getpid ());
	  if (strchr (flags, 'N') != NULL)
	    {
	      register_core_plugins = FALSE;
	      register_ladspa_plugins = FALSE;
	      register_scripts = FALSE;
	    }
	  if (strchr (flags, 'p'))
	    register_core_plugins = TRUE;
	  if (strchr (flags, 'P'))
	    register_core_plugins = FALSE;
	  if (strchr (flags, 'l'))
	    register_ladspa_plugins = TRUE;
	  if (strchr (flags, 'L'))
	    register_ladspa_plugins = FALSE;
	  if (strchr (flags, 's'))
	    register_scripts = TRUE;
	  if (strchr (flags, 'S'))
	    register_scripts = FALSE;
	  if (strchr (flags, 'f'))
	    {
	      GLogLevelFlags fatal_mask = g_log_set_always_fatal (GLogLevelFlags (G_LOG_FATAL_MASK));
	      fatal_mask = GLogLevelFlags (fatal_mask | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
	      g_log_set_always_fatal (fatal_mask);
	    }
	  if (strchr (flags, 'd') != NULL)
	    {
              bst_developer_hints = TRUE;
	      bst_debug_extensions = TRUE;
	      g_message ("enabling possibly harmful debugging extensions due to '-:d'");
	    }
	  argv[i] = NULL;
	}
      else if (strcmp ("--devel", argv[i]) == 0)
	{
	  bst_developer_hints = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("--force-xkb", argv[i]) == 0)
	{
	  arg_force_xkb = TRUE;
          argv[i] = NULL;
	}
      else if (strcmp ("--rewrite-bse-file", argv[i]) == 0)
        {
          rewrite_bse_file = true;
          argv[i] = NULL;
        }
      else if (strcmp (argv[i], "-n") == 0 && i + 1 < argc)
        { /* handled by priviledged launcher */
          argv[i++] = NULL;
          argv[i] = NULL;
        }
      else if (strncmp (argv[i], "-n=", 3) == 0 ||
               (strncmp (argv[i], "-n", 2) == 0 && argv[i][3] >= '0' && argv[i][3] <= '9') ||
               strcmp (argv[i], "-N") == 0)
        { /* handled by priviledged launcher */
          argv[i] = NULL;
        }
      else if (strcmp ("-h", argv[i]) == 0 ||
	       strcmp ("--help", argv[i]) == 0)
	{
	  bst_print_blurb ();
          argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("-v", argv[i]) == 0 ||
	       strcmp ("--version", argv[i]) == 0)
	{
	  bst_exit_print_version ();
	  argv[i] = NULL;
	  exit (0);
	}
      else if (strcmp ("--skinrc", argv[i]) == 0 ||
	       strncmp ("--skinrc=", argv[i], 9) == 0)
        {
          const char *arg = argv[i][9 - 1] == '=' ? argv[i] + 9 : (argv[i + 1] ? argv[i + 1] : "");
          bst_skin_config_set_rcfile (arg);
        }
      else if (strcmp ("--print-dir", argv[i]) == 0 ||
	       strncmp ("--print-dir=", argv[i], 12) == 0)
	{
	  const char *arg = argv[i][12 - 1] == '=' ? argv[i] + 12 : (argv[i + 1] ? argv[i + 1] : "");
          char *freeme = NULL;
	  if (strcmp (arg, "prefix") == 0)
	    g_print ("%s\n", BST_PATH_PREFIX);
	  else if (strcmp (arg, "docs") == 0)
	    g_print ("%s\n", BST_PATH_DOCS);
	  else if (strcmp (arg, "images") == 0)
	    g_print ("%s\n", BST_PATH_IMAGES);
	  else if (strcmp (arg, "locale") == 0)
	    g_print ("%s\n", BST_PATH_LOCALE);
	  else if (strcmp (arg, "skins") == 0)
	    g_print ("%s\n", freeme = BST_STRDUP_SKIN_PATH ());
	  else if (strcmp (arg, "keys") == 0)
	    g_print ("%s\n", BST_PATH_KEYS);
	  else if (strcmp (arg, "ladspa") == 0)
	    g_print ("%s\n", BSE_PATH_LADSPA);
	  else if (strcmp (arg, "plugins") == 0)
	    g_print ("%s\n", BSE_PATH_PLUGINS);
	  else if (strcmp (arg, "samples") == 0)
	    g_print ("%s\n", bse_server_get_sample_path (BSE_SERVER));
	  else if (strcmp (arg, "effects") == 0)
	    g_print ("%s\n", bse_server_get_effect_path (BSE_SERVER));
	  else if (strcmp (arg, "scripts") == 0)
	    g_print ("%s\n", bse_server_get_script_path (BSE_SERVER));
	  else if (strcmp (arg, "instruments") == 0)
	    g_print ("%s\n", bse_server_get_instrument_path (BSE_SERVER));
	  else if (strcmp (arg, "demo") == 0)
	    g_print ("%s\n", bse_server_get_demo_path (BSE_SERVER));
	  else
	    {
	      if (arg[0])
                g_message ("no such resource path: %s", arg);
	      g_message ("supported resource paths: prefix, docs, images, keys, locale, skins, ladspa, plugins, scripts, effects, instruments, demo, samples");
	    }
          g_free (freeme);
	  exit (0);
	}
      else if (strcmp ("--bse-latency", argv[i]) == 0 ||
               strncmp ("--bse-latency=", argv[i], 14) == 0)
        {
          gchar *equal = argv[i] + 13;
          may_auto_update_bse_rc_file = FALSE;
          if (*equal != '=')
            i++;
          /* leave args for BSE */
        }
      else if (strcmp ("--bse-mixing-freq", argv[i]) == 0 ||
               strncmp ("--bse-mixing-freq=", argv[i], 18) == 0)
        {
          gchar *equal = argv[i] + 17;
          may_auto_update_bse_rc_file = FALSE;
          if (*equal != '=')
            i++;
          /* leave args for BSE */
        }
      else if (strcmp ("--bse-control-freq", argv[i]) == 0 ||
               strncmp ("--bse-control-freq=", argv[i], 19) == 0)
        {
          gchar *equal = argv[i] + 18;
          may_auto_update_bse_rc_file = FALSE;
          if (*equal != '=')
            i++;
          /* leave args for BSE */
        }
      else if (strcmp ("--bse-driver-list", argv[i]) == 0)
        {
          initialize_bse_and_exit = TRUE;
          /* leave args for BSE */
        }
      else if (strcmp ("-p", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = (char*) "--bse-pcm-driver";
        }
      else if (strcmp ("-m", argv[i]) == 0)
        {
          /* modify args for BSE */
          argv[i] = (char*) "--bse-midi-driver";
        }
    }
  gxk_param_set_devel_tips (bst_developer_hints);

  e = 1;
  for (i = 1; i < argc; i++)
    if (argv[i])
      {
        argv[e++] = argv[i];
        if (i >= e)
          argv[i] = NULL;
      }
  *argc_p = e;
  if (initialize_bse_and_exit)
    {
      Bse::init_async (argc_p, argv, "BEAST");
      exit (0);
    }
}

static void G_GNUC_NORETURN
bst_exit_print_version (void)
{
  const gchar *c;
  gchar *freeme = NULL;
  /* hack: start BSE, so we can query it for paths, works since we immediately exit() afterwards */
  Bse::init_async (NULL, NULL, "BEAST");
  sfi_glue_context_push (Bse::init_glue_context ("BEAST", bst_main_loop_wakeup));
  g_print ("BEAST version %s (%s)\n", BST_VERSION, BST_VERSION_HINT);
  g_print ("Libraries: ");
  g_print ("GLib %u.%u.%u", glib_major_version, glib_minor_version, glib_micro_version);
  g_print (", SFI %s", BST_VERSION);
  g_print (", BSE %s", BST_VERSION);
  c = bse_server_get_vorbis_version (BSE_SERVER);
  if (c)
    g_print (", %s", c);
  c = bse_server_get_mp3_version (BSE_SERVER);
  if (c)
    g_print (", %s", c);
  g_print (", GTK+ %u.%u.%u", gtk_major_version, gtk_minor_version, gtk_micro_version);
#ifdef BST_WITH_XKB
  g_print (", XKBlib");
#endif
  g_print (", GXK %s", BST_VERSION);
  g_print ("\n");
  g_print ("Compiled for %s %s SSE plugins.\n", BST_ARCH_NAME, BSE_WITH_MMX_SSE ? "with" : "without");
  g_print ("Intrinsic code selected according to runtime CPU detection:\n");
  g_print ("%s", Rapicorn::cpu_info().c_str());
  g_print ("\n");
  g_print ("Prefix:          %s\n", BST_PATH_PREFIX);
  g_print ("Doc Path:        %s\n", BST_PATH_DOCS);
  g_print ("Image Path:      %s\n", BST_PATH_IMAGES);
  g_print ("Locale Path:     %s\n", BST_PATH_LOCALE);
  g_print ("Keyrc Path:      %s\n", BST_PATH_KEYS);
  g_print ("Skin Path:       %s\n", freeme = BST_STRDUP_SKIN_PATH());
  g_print ("Sample Path:     %s\n", bse_server_get_sample_path (BSE_SERVER));
  g_print ("Script Path:     %s\n", bse_server_get_script_path (BSE_SERVER));
  g_print ("Effect Path:     %s\n", bse_server_get_effect_path (BSE_SERVER));
  g_print ("Instrument Path: %s\n", bse_server_get_instrument_path (BSE_SERVER));
  g_print ("Demo Path:       %s\n", bse_server_get_demo_path (BSE_SERVER));
  g_print ("Plugin Path:     %s\n", bse_server_get_plugin_path (BSE_SERVER));
  g_print ("LADSPA Path:     %s:$LADSPA_PATH\n", bse_server_get_ladspa_path (BSE_SERVER));
  g_print ("\n");
  g_print ("BEAST comes with ABSOLUTELY NO WARRANTY.\n");
  g_print ("You may redistribute copies of BEAST under the terms of\n");
  g_print ("the GNU Lesser General Public License which can be found in\n");
  g_print ("the BEAST source package. Sources, examples and contact\n");
  g_print ("information are available at http://beast.testbit.eu/.\n");
  g_free (freeme);
  exit (0);
}

static void
bst_print_blurb (void)
{
  g_print ("Usage: beast [options] [files...]\n");
  /*        12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
#ifdef BST_WITH_XKB
  g_print ("  --force-xkb             force XKB keytable queries\n");
#endif
  g_print ("  --skinrc[=FILENAME]     Skin resource file name\n");
  g_print ("  --print-dir[=RESOURCE]  Print the directory for a specific resource\n");
  g_print ("  --merge                 Merge the following files into the previous project\n");
  g_print ("  --devel                 Enrich the GUI with hints useful for developers,\n");
  g_print ("                          enable unstable plugins and experimental code\n");
  g_print ("  -h, --help              Show this help message\n");
  g_print ("  -v, --version           Print version and file paths\n");
  g_print ("  -n NICELEVEL            Run with priority NICELEVEL (for suid wrapper beast)\n");
  g_print ("  -N                      Disable renicing\n");
  g_print ("  --display=DISPLAY       X server for the GUI; see X(1)\n");
  g_print ("  --bse-latency=USECONDS  Specify synthesis latency in milliseconds\n");
  g_print ("  --bse-mixing-freq=FREQ  Specify synthesis mixing frequency in Hz \n");
  g_print ("  --bse-control-freq=FREQ Specify control frequency in Hz\n");
  g_print ("  --bse-force-fpu         Disable loading of SSE or similarly optimized plugins\n");
  g_print ("  --bse-pcm-driver DRIVERCONF\n");
  g_print ("  -p DRIVERCONF           Try to use the PCM driver DRIVERCONF, multiple\n");
  g_print ("                          options may be supplied to try a variety of\n");
  g_print ("                          drivers. Uunless -p auto is given, only the\n");
  g_print ("                          drivers listed by -p options are used; each\n");
  g_print ("                          DRIVERCONF consists of a driver name and may be\n");
  g_print ("                          assigned an optional comma seperated list of\n");
  g_print ("                          arguments, e.g.: -p oss=/dev/dsp2,rw\n");
  g_print ("  --bse-midi-driver DRIVERCONF\n");
  g_print ("  -m DRIVERCONF           Try to use the MIDI driver DRIVERCONF, multiple\n");
  g_print ("                          options may be specified similarly to the\n");
  g_print ("                          option handling for --bse-pcm-driver\n");
  g_print ("  --bse-driver-list       List available PCM and MIDI drivers\n");
  g_print ("Development Options:\n");
  g_print ("  -:[Flags]               [Flags] can be any combination of:\n");
  g_print ("                          f - fatal warnings\n");
  g_print ("                          N - disable script and plugin registration\n");
  g_print ("                          p - enable core plugin registration\n");
  g_print ("                          P - disable core plugin registration\n");
  g_print ("                          l - enable LADSPA plugin registration\n");
  g_print ("                          L - disable LADSPA plugin registration\n");
  g_print ("                          s - enable script registration\n");
  g_print ("                          S - disable script registration\n");
  g_print ("                          d - enable debugging extensions (harmfull)\n");
  g_print ("Gtk+ Options:\n");
  g_print ("  --gtk-debug=FLAGS       Gtk+ debugging flags to enable\n");
  g_print ("  --gtk-no-debug=FLAGS    Gtk+ debugging flags to disable\n");
  g_print ("  --gtk-module=MODULE     Load additional Gtk+ modules\n");
  g_print ("  --gdk-debug=FLAGS       Gdk debugging flags to enable\n");
  g_print ("  --gdk-no-debug=FLAGS    Gdk debugging flags to disable\n");
  g_print ("  --g-fatal-warnings      Make warnings fatal (abort)\n");
  g_print ("  --sync                  Do all X calls synchronously\n");
}

void
beast_show_about_box (void)
{
  /* contributor/author names do not get i18n markup. instead, their names are to be
   * written using utf-8 encoding, where non-ascii characters are entered using
   * hexadecimal escape sequences. for instance: "Ville P\xc3\xa4tsi"
   */
  const gchar *contributors[] = {
    /* graphics */
    //"Artem Popov",
    "Cyria Arweiler",
    "Jakub Steiner",
    "Tuomas Kuosmanen",
    "Ville P\xc3\xa4tsi",
    /* general code and fixes */
    "Alessio Treglia",
    "Alper Ersoy",
    "Ben Collver",
    "Hanno Behrens",
    "Jonh Wendell",
    "Nedko Arnaudov",
    "Rasmus Kaj",
    "Sam Hocevar",
    "Stefan Westerfeld",
    "Sven Herzberg",
    "Tim Janik",
    /* plugins & instruments */
    "\xd0\x90\xd1\x80\xd1\x82\xd0\xb5\xd0\xbc \xd0\x9f\xd0\xbe\xd0\xbf\xd0\xbe\xd0\xb2", /* "Artem Popov", */
    "David A. Bartold",
    //"Tim Janik",
    //"Stefan Westerfeld",
    "William DeVore",
    "Krzysztof Foltman",
    /* translations */
    "Adam Weinberger",
    "Alexandre Prokoudine",
    "Amanpreet Singh Alam",
    //"Artem Popov",
    "Bruno Brouard",
    "Christian Neumair",
    "Christian Rose",
    "Christophe Merlet",
    "Daniel Nylander",
    "Danilo Segan",
    "David Lodge",
    "Dirk Janik",
    "Duarte Loreto",
    "Dulmandakh Sukhbaatar",
    "Francisco Javier F. Serrador",
    "Gareth Owen",
    "Gil Forcada",
    "Hendrik Richter",
    "Hizkuntza Politikarako Sailburuordetza",
    "Iassen Pramatarov",
    // "I\xc3\261aki Larra\xc3\261aga Murgotio",
    "Ismael Andres Rubio Rojas",
    "Jens Seidel",
    "Joe Hansen",
    "Jorge Gonzalez",
    "Kjartan Maraas",
    "Kees van den Broek",
    "Kostas Papadimas",
    "Laurent Dhima",
    "Leonardo Ferreira Fontenelle",
    "Mario Blättermann",
    "Maxim V. Dziumanenko",
    "Metin Amiroff",
    "Miloslav Trmac",
    "Moritz Mekelburger",
    "Pawan Chitrakar",
    "Michele Petrecca",
    "Raphael Higino",
    "Robert Sedak",
    "Satoru SATOH",
    "Steve Murphy",
    "Tino Meinen",
    "Veeven",
    "Vincent van Adrighem",
    "Xavier Conde Rueda",
    "Yelitza Louze",
    "Yannig Marchegay",
    NULL
  };
  if (!GTK_WIDGET_VISIBLE (beast_splash))
    {
      bst_splash_set_title (beast_splash, _("BEAST About"));
      bst_splash_update_entity (beast_splash, Rapicorn::string_format (_("BEAST Version %s"), BST_VERSION));
      bst_splash_update_item (beast_splash, _("Contributions made by:"));
      bst_splash_animate_strings (beast_splash, contributors);
    }
  gxk_widget_showraise (beast_splash);
}
