#ifndef MINUNIT_H
#define MINUNIT_H
/* file: minunit.h */
 #define mu_assert(message, test) do { if (!(test)) { p_message = message; goto cleanup;}} while (0)
 #define mu_run_test(test) do { char *p_message = test(); tests_run++; if (p_message) return  p_message;} while (0)
extern int tests_run;
#endif // MINUNIT_H
