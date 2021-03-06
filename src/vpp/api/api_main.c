#include "vat.h"

vat_main_t vat_main;

void
vat_suspend (vlib_main_t * vm, f64 interval)
{
  vlib_process_suspend (vm, interval);
}

static u8 *
format_api_error (u8 * s, va_list * args)
{
  vat_main_t *vam = va_arg (*args, vat_main_t *);
  i32 error = va_arg (*args, u32);
  uword *p;

  p = hash_get (vam->error_string_by_error_number, -error);

  if (p)
    s = format (s, "%s", p[0]);
  else
    s = format (s, "%d", error);
  return s;
}


static void
init_error_string_table (vat_main_t * vam)
{

  vam->error_string_by_error_number = hash_create (0, sizeof (uword));

#define _(n,v,s) hash_set (vam->error_string_by_error_number, -v, s);
  foreach_vnet_api_error;
#undef _

  hash_set (vam->error_string_by_error_number, 99, "Misc");
}

static clib_error_t *
api_main_init (vlib_main_t * vm)
{
  vat_main_t *vam = &vat_main;

  vam->vlib_main = vm;
  vam->my_client_index = (u32) ~ 0;
  init_error_string_table (vam);
  vat_api_hookup (vam);
  return 0;
}

VLIB_INIT_FUNCTION (api_main_init);

static clib_error_t *
api_command_fn (vlib_main_t * vm,
		unformat_input_t * input, vlib_cli_command_t * cmd)
{
  vat_main_t *vam = &vat_main;
  unformat_input_t _input;
  uword c;
  u8 *cmdp, *argsp, *this_cmd;
  uword *p;
  u32 arg_len;
  int rv;
  int (*fp) (vat_main_t *);
  api_main_t *am = &api_main;

  vam->vl_input_queue = am->shmem_hdr->vl_input_queue;

  vec_reset_length (vam->inbuf);
  vam->input = &_input;

  while (((c = unformat_get_input (input)) != '\n') &&
	 (c != UNFORMAT_END_OF_INPUT))
    vec_add1 (vam->inbuf, c);

  /* Add 1 octet's worth of extra space in case there are no args... */
  vec_add1 (vam->inbuf, 0);

  /*$$$$ reinstall macro evaluator */

  /* Split input into cmd + args */
  this_cmd = cmdp = vam->inbuf;

  while (cmdp < (this_cmd + vec_len (this_cmd)))
    {
      if (*cmdp == ' ' || *cmdp == '\t' || *cmdp == '\n')
	{
	  cmdp++;
	}
      else
	break;
    }

  argsp = cmdp;
  while (argsp < (this_cmd + vec_len (this_cmd)))
    {
      if (*argsp != ' ' && *argsp != '\t' && *argsp != '\n')
	{
	  argsp++;
	}
      else
	break;
    }
  *argsp++ = 0;

  while (argsp < (this_cmd + vec_len (this_cmd)))
    {
      if (*argsp == ' ' || *argsp == '\t' || *argsp == '\n')
	{
	  argsp++;
	}
      else
	break;
    }

  /* Blank input line? */
  if (*cmdp == 0)
    return 0;

  p = hash_get_mem (vam->function_by_name, cmdp);
  if (p == 0)
    {
      return clib_error_return (0, "'%s': function not found\n", cmdp);
    }

  arg_len = strlen ((char *) argsp);

  unformat_init_string (vam->input, (char *) argsp, arg_len);
  fp = (void *) p[0];

  rv = (*fp) (vam);

  if (rv < 0)
    {
      unformat_free (vam->input);
      return clib_error_return (0,
				"%s error: %U\n", cmdp,
				format_api_error, vam, rv);

      if (vam->regenerate_interface_table)
	{
	  vam->regenerate_interface_table = 0;
	  api_sw_interface_dump (vam);
	}
    }
  unformat_free (vam->input);
  return 0;
}

/* *INDENT-OFF* */
VLIB_CLI_COMMAND (api_command, static) =
{
  .path = "binary-api",
  .short_help = "binary-api <name> [<args>]",
  .function = api_command_fn,
};
/* *INDENT-ON* */

void
api_cli_output (void *notused, const char *fmt, ...)
{
  va_list va;
  vat_main_t *vam = &vat_main;
  vlib_main_t *vm = vam->vlib_main;
  vlib_process_t *cp = vlib_get_current_process (vm);
  u8 *s;

  va_start (va, fmt);
  s = va_format (0, fmt, &va);
  va_end (va);

  /* Terminate with \n if not present. */
  if (vec_len (s) > 0 && s[vec_len (s) - 1] != '\n')
    vec_add1 (s, '\n');

  if ((!cp) || (!cp->output_function))
    fformat (stdout, "%v", s);
  else
    cp->output_function (cp->output_function_arg, s, vec_len (s));

  vec_free (s);
}

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "gnu")
 * End:
 */
