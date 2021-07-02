How do you use this?

Place the fixsplinks directory in ~/.drush/commands (create it if you don't have one)
and run `drush help fsl'.

Use `drush fsl to fix all links like http://www.heerlen.sp.nl
to https://weert.sp.nl in all text fields (node body and field API) and
all custom blocks on the site.

Be sure to backup your database before running this command, as that is the
only way to undo changes.
