#!/usr/bin/env python3
import fastest
import IF_tests as IF

fastest.default_runner.set_backend(IF)

fastest.run_log_all()
