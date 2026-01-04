# Hi! :wave:

Thanks for your interest in contributing to the libexpat project or "Expat"! :+1:

> [!TIP]
> TL;DR: **The best move before creating a pull request**
> that is big, complex, and/or controversial
> is to [open a new issue](https://github.com/libexpat/libexpat/issues)
> and discuss options moving forward :pray: :beers:

Below, I'll document what Expat needs and does not need, at this time.


# What Expat needs (and does not need) in general

Expat is:

- in maintenance mode,
- unfunded,
- understaffed,
- complex,
- 25+ years old in some regards,
- meant to support parsing of XML files that exceed available RAM,
- written in the memory-unsafe language C99,
- is [used in many places](https://libexpat.github.io/doc/users/),
  both in [hardware](https://libexpat.github.io/doc/users/#hardware)
  and in [software](https://libexpat.github.io/doc/users/),
- implementing [XML 1.0 *Fourth* Edition](https://www.w3.org/TR/2006/REC-xml-20060816/),
- targeting environments that are relevant today
  and/or up to 5 years into the past.


## What kinds of pull requests will be welcome

Pull requests will be most welcome when they:

- are small,
- are easy to review,
- have low risk,
- have had GitHub Actions CI run and all-green already
  in your public fork repository,
- help both you and the libexpat project :tada:

The need for additional tests or CI will depend on the specific pull request.


## What kinds of pull requests will *not* be welcome

Pull requests will *not* be welcome when:

- the changes are big, complex, high risk, controversial
  and/or hard to review,
- the exact change has been said "no" to in the past,
- the changes break API or ABI backwards compatibility,
- it fixes an issue for an unsupported
  (or no-longer supported) environment, and/or
- it introduces new features
  (without prior discussion and agreement
  in a dedicated issue).

Specific examples of things known to *not* be welcome:

- Mass-changing things.
- Fixing things for some stone age version of Visual Studio.
- Adding support to ignore or recover from malformed XML.
- Adding support for XML streaming.
- Making the C code compile with a C++ compiler.
- Adding support for XML 1.0r5 without prior discussion.
- Adding support for XML 1.1 in general.
- Adding additional build systems.


# What Expat needs on a technical level

- All new C code and changes to existing C code must:
  - pass all existing CI,
  - conform to C99,
  - check returns from `malloc`/`realloc` for `NULL`,
  - not use recursion (when it can be controlled by user input),
  - have integer overflow in mind,
  - have undefined behavior in mind, and
  - have security in mind in general.
- A commit should be about one — and just one — of these things:
  - fixing a bug,
  - improving something,
  - adding something new,
  - doing a refactoring, or
  - adjusting whitespace.


# Providing feedback

Thanks for your interest and attention! :+1:

I'm happy about [constructive feedback](mailto:sebastian@pipping.org) to this document — thank you! :pray:
