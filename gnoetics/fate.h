/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef __FATE_H__
#define __FATE_H__


void     fate_seed           (unsigned seed);

void     fate_seed_from_time (void);

unsigned fate_random         (unsigned N);


#endif /* __FATE_H__ */

